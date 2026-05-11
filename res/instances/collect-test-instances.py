#!/usr/bin/env python3
import json
import sys
import argparse
from collections import defaultdict, Counter
import math
import matplotlib
matplotlib.use('Agg')  # Headless (no display needed on HPC/CI)
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np


# =============================================================================
# USER VARIABLES - Tune these easily!
# =============================================================================
"""
All tunable parameters are here. Adjust without touching code below.

TIME_BUCKETS (dict[str, float]): Exclusive time thresholds (s).
  - Keys: category names (used in stats/output).
  - Values: upper bound (< thresh).
  - Adds implicit 'slow' > last threshold.
  Example: {'fast1s':1.0, 'fast30s':30.0} -> <1s, 1-30s, >=30s.

SAMPLE_FRACS (dict[str, float]): Sampling fraction per time category.
  - Keys: match TIME_BUCKETS keys.
  - Values: fraction (0.0-1.0); min 1 instance.
  - Samples slowest first (reverse sorted by wtime).
  - 'fast1s' implicitly 1.0 (all).

SMALL_LEAVES_MAX (int): Max leaves for "hard small" category.

HARD_SMALL_VALID_N (int): Num slowest Valid <= SMALL_LEAVES_MAX.
HARD_SMALL_TIMEOUT_N (int): Num smallest Timeout <= SMALL_LEAVES_MAX.

BUCKET_START (int), BUCKET_FACTOR (float): Leaf bucketing.
  - Buckets: START, START*FACTOR, START*FACTOR^2, ...
  - E.g., 50*2 -> 50,100,200,400,...

BUCKET_SIZES (dict[int, int]): Max instances per bucket.
  - Prefers Timeout + slowest Valid (half each).

HUGE_LEAVES_MIN (int), HUGE_TIME_MAX (float): Detect fast-solved giants.
"""

# Time thresholds (seconds)
TIME_BUCKETS = {
    'fast1s': 1.0,           # All <1s
    'fast30s': 30.0,         # Sample up to 30s
    'fast1m': 60.0,          # Sample up to 1min
    'fast5m': 300.0,         # Sample up to 5min
    'fast15m': 900.0,        # Sample up to 15min
}

# Sampling fractions (for non-all buckets)
SAMPLE_FRACS = {
    'fast1s': 1,           # 50% <1s
    'fast30s': 0.5,         # 5% of 1-30s (slowest)
    'fast1m': 0.2,          # 1% of 30s-1m (slowest)
    'fast5m': 0.1,         # 0,5% of 30s-5m (slowest)
    'fast15m': 0.01,        # 0,1% of 5-15m (slowest)
}

# Sampling fractions for SLOW categories only (>=15m)
SLOW_SAMPLE_FRACS = {
    'fast15m': 0.05,         # 5% of 5-15m
    'slow': 0.02,            # 2% of >15m
}

# Hard small instances (<= small_leaves_max)
SMALL_LEAVES_MAX = 30
HARD_SMALL_VALID_N = 10      # Top N slowest Valid <= small_leaves_max
HARD_SMALL_TIMEOUT_N = 10    # Top N smallest Timeout <= small_leaves_max

# Bucket config for larger instances
BUCKET_START = 50
BUCKET_FACTOR = 2            # 50,100,200,...
BUCKET_SIZES = {             # Override per-bucket if needed
    50: 15,
    100: 10,
    200: 5,
    400: 3,
    800: 2,
    1600: 1,
}

# Huge solved detection
HUGE_LEAVES_MIN = 400
HUGE_TIME_MAX = 3600         # Solved <1h

# =============================================================================
# FUNCTIONS (with docstrings, types)
# =============================================================================

def load_summary(filename: str) -> list[dict]:
    """
    Load JSON lines from summary file.

    Args:
        filename (str): Path to summary.json.

    Returns:
        list[dict]: List of instance dicts.

    Raises:
        FileNotFoundError, json.JSONDecodeError.
    """
    data = []
    with open(filename, 'r') as f:
        for line in f:
            data.append(json.loads(line.strip()))
    return data

def categorize_by_time(valid_data: list[dict], time_buckets: dict[str, float]) -> dict[str, list[dict]]:
    """
    Categorize valid instances into exclusive time buckets (slowest first).

    Args:
        valid_data (list[dict]): Valid instances only.
        time_buckets (dict[str, float]): {cat_name: upper_thresh}.

    Returns:
        dict[str, list[dict]]: {cat: sorted_list (desc wtime)}.
        Includes 'slow' for > max threshold.
    """
    categories = {name: [] for name in time_buckets}
    prev_thresh = 0.0
    for name, thresh in time_buckets.items():
        cat = [d for d in valid_data if prev_thresh <= d['s_wtime'] < thresh]
        categories[name] = sorted(cat, key=lambda d: d['s_wtime'], reverse=True)
        prev_thresh = thresh
    # Fast: ALL < max threshold (combined)
    max_fast = list(time_buckets.values())[-1]
    categories['fast_all'] = [d for d in valid_data if d['s_wtime'] < max_fast]
    # Slow (> last threshold)
    categories['slow'] = [d for d in valid_data if d['s_wtime'] >= max_fast]
    return categories

def sample_slow_categories(categories: dict[str, list[dict]], sample_fracs: dict[str, float]) -> list[dict]:
    """
    Sample slowest from SLOW categories only (>=15m).

    Args:
        categories (dict[str, list[dict]]): From categorize_by_time.
        sample_fracs (dict[str, float]): {cat: frac} for slow cats.

    Returns:
        list[dict]: Flattened selected.
    """
    selected = []
    for cat_name, frac in sample_fracs.items():
        if cat_name in categories and cat_name != 'fast_all':
            n_sample = max(1, int(frac * len(categories[cat_name])))
            selected.extend(categories[cat_name][:n_sample])
    return selected


def sample_categories(categories: dict[str, list[dict]], sample_fracs: dict[str, float]) -> list[dict]:
    """
    Sample slowest from time categories based on fractions.
    Used for fast buckets (TIME_BUCKETS) via SAMPLE_FRACS.
    Note: categories['fast_all'] is intentionally excluded here;
    use directly if you want ALL fast instances instead of sampling.

    Args:
        categories (dict[str, list[dict]]): From categorize_by_time.
        sample_fracs (dict[str, float]): {cat: frac}.

    Returns:
        list[dict]: Flattened selected (may have dups, dedup later).
    """
    selected = []
    for cat_name, frac in sample_fracs.items():
        if cat_name in categories:
            n_sample = max(1, int(frac * len(categories[cat_name])))
            selected.extend(categories[cat_name][:n_sample])
    return selected

def get_bucket(leaves: int, start: int = 50, factor: float = 2) -> int:
    """
    Compute leaf bucket.

    Args:
        leaves (int): s_num_leaves.
        start, factor: Bucketing params.

    Returns:
        int: Bucket value (or 'small' if <=35).
    """
    bucket = start
    while bucket < leaves:
        bucket = math.ceil(bucket * factor)
    return bucket
def select_hard_small(data: list[dict], small_leaves_max: int,
                      n_valid: int, n_timeout: int) -> list[dict]:
    """
    Hard small: slowest Valid + smallest Timeouts <= small_leaves_max.

    Args:
        data (list[dict]): All data.
        small_leaves_max (int), n_valid (int), n_timeout (int).

    Returns:
        list[dict]: Combined selection.
    """
    valid_small = [d for d in data if d['s_result'] == 'Valid' and d['s_num_leaves'] <= small_leaves_max]
    timeout_small = [d for d in data if d['s_result'] == 'Timeout' and d['s_num_leaves'] <= small_leaves_max]
    hard_valid = sorted(valid_small, key=lambda d: d['s_wtime'], reverse=True)[:n_valid]
    hard_timeout = sorted(timeout_small, key=lambda d: d['s_num_leaves'])[:n_timeout]
    return hard_valid + hard_timeout

def select_buckets(data: list[dict], bucket_sizes: dict[int, int]) -> tuple[list[dict], dict]:
    """
    Select from leaf buckets (Timeout + slow Valid).

    Args:
        data (list[dict]): All data.
        bucket_sizes (dict[int,int]): {bucket: max_n}.

    Returns:
        list[dict]: Selected across buckets.
    """
    buckets = defaultdict(list)
    for d in data:
        b = get_bucket(d['s_num_leaves'])  # Now always int >=50
        if b >= BUCKET_START:  # Skip tiny
            buckets[b].append(d)

    selected = []
    bucket_stats = {}
    print("\nLeaf buckets:")
    for b in sorted(buckets.keys()):  # Safe: all int
        bucket_data = buckets[b]
        n_total = len(bucket_data)
        timeouts = [d for d in bucket_data if d['s_result'] == 'Timeout']
        n_timeouts = len(timeouts)
        valid_bucket = [d for d in bucket_data if d['s_result'] == 'Valid']
        slow_valid = sorted(valid_bucket, key=lambda d: d['s_wtime'], reverse=True)
        print(f"  {b}: {n_total} total ({n_timeouts} timeout)")

        size = bucket_sizes.get(b, 1)
        bucket_sel = timeouts[:size//2] + slow_valid[:size//2]
        selected.extend(bucket_sel[:size])
        bucket_stats[b] = {'total': n_total, 'timeouts': n_timeouts, 'selected': len(bucket_sel)}

    return selected, bucket_stats
def find_huge_solved(valid_data: list[dict], huge_leaves_min: int,
                     huge_time_max: float) -> list[dict]:
    """Fast-solved huge instances."""
    huge = [d for d in valid_data
            if d['s_num_leaves'] >= huge_leaves_min and d['s_wtime'] <= huge_time_max]
    return sorted(huge, key=lambda d: d['s_num_leaves'], reverse=True)[:5]

def dedup(selected: list[dict]) -> list[dict]:
    """Deduplicate by s_idigest."""
    seen = set()
    unique = []
    for d in selected:
        key = d['s_idigest']
        if key not in seen:
            seen.add(key)
            unique.append(d)
    return unique

def print_stats(data: list[dict], valid_data: list[dict]):
    """Print data overview."""
    print(f"Loaded {len(data)} entries ({len(valid_data)} valid).")
    leaves = [d['s_num_leaves'] for d in data]
    times = [d['s_wtime'] for d in valid_data]
    print(f"Leaves: min={min(leaves)}, max={max(leaves)}, median={sorted(leaves)[len(leaves)//2]}")
    if times:
        print(f"Times: min={min(times):.2f}s, median={sorted(times)[len(times)//2]:.2f}s, max={max(times):.2f}s")

def write_lst(selected: list[dict], output_file: str) -> None:
    """Write s:ID lines to file."""
    with open(output_file, 'w') as f:
        for d in selected:
            f.write(f"s:{d['s_idigest']}\n")

def print_selection_stats(data: list[dict], selected: list[dict]) -> None:
    """
    Print a breakdown of selected instances: counts, time ranges,
    and timeout share per leaf-size group.

    Args:
        data (list[dict]): All instances (for context).
        selected (list[dict]): The filtered selection.
    """
    TIMEOUT_WTIME = 1800.0  # Sentinel display value for timeouts

    print("\n" + "=" * 60)
    print("SELECTION STATISTICS")
    print("=" * 60)

    total = len(selected)
    valid = [d for d in selected if d['s_result'] == 'Valid']
    timeouts = [d for d in selected if d['s_result'] == 'Timeout']

    print(f"Total selected : {total}")
    print(f"  Valid        : {len(valid)}")
    print(f"  Timeouts     : {len(timeouts)}")

    # Time distribution of valid ones
    if valid:
        wtimes = sorted(d['s_wtime'] for d in valid)
        print(f"\nValid runtimes (prior run):")
        print(f"  min    : {wtimes[0]:.3f}s")
        print(f"  median : {wtimes[len(wtimes)//2]:.3f}s")
        print(f"  p90    : {wtimes[int(len(wtimes)*0.90)]:.3f}s")
        print(f"  max    : {wtimes[-1]:.3f}s")

        # DYNAMIC BUCKET COUNTS from TIME_BUCKETS
        prev_thresh = 0.0
        bucket_counts = {}
        for name, thresh in TIME_BUCKETS.items():
            count = sum(1 for t in wtimes if prev_thresh <= t < thresh)
            bucket_counts[name] = count
            print(f"  {name:>8} : {count}")
            prev_thresh = thresh

        # Slow (> last threshold)
        slow_count = sum(1 for t in wtimes if t >= prev_thresh)
        print(f"  slow{'>'+str(prev_thresh)+'s':>5} : {slow_count}")

    # Per-leaf-bucket breakdown
    buckets = defaultdict(lambda: {'valid': [], 'timeout': 0})
    leaf_edges = [0, 10, 20, 35, 50, 75, 100, 200, 400, 800, 1600, float('inf')]
    labels = ['≤10','11-20','21-35','36-50','51-75','76-100',
              '101-200','201-400','401-800','801-1600','>1600']

    for d in selected:
        leaves = d['s_num_leaves']
        for i in range(len(leaf_edges) - 1):
            if leaf_edges[i] < leaves <= leaf_edges[i+1]:
                if d['s_result'] == 'Valid':
                    buckets[labels[i]]['valid'].append(d['s_wtime'])
                else:
                    buckets[labels[i]]['timeout'] += 1
                break

    print(f"\n{'Leaf range':<12} {'Count':>6} {'Timeouts':>9} "
          f"{'Min(s)':>8} {'Med(s)':>8} {'Max(s)':>8}")
    print("-" * 60)
    for label in labels:
        b = buckets[label]
        vtimes = sorted(b['valid'])
        n = len(vtimes) + b['timeout']
        if n == 0:
            continue
        min_t  = f"{vtimes[0]:.2f}"  if vtimes else "TIMEOUT"
        med_t  = f"{vtimes[len(vtimes)//2]:.2f}" if vtimes else "TIMEOUT"
        max_t  = f"{vtimes[-1]:.2f}" if vtimes else "TIMEOUT"
        print(f"{label:<12} {n:>6} {b['timeout']:>9} "
              f"{min_t:>8} {med_t:>8} {max_t:>8}")

    # ETA prediction
    if valid:
        p90_time = wtimes[int(len(wtimes)*0.90)]
        n_parallel = 16  # Tune to your HPC
        est_total = (len(valid) * p90_time / n_parallel / 3600)  # hours
        print(f"\nPREDICTED RUNTIME (p90={p90_time:.0f}s, {n_parallel}p): "
              f"~{est_total:.1f}h total")

    print("=" * 60)


def plot_selection(data: list[dict], selected: list[dict],
                   output_path: str = "selection_stats.png") -> None:
    """
    Log-scale scatter plot: leaves vs prior wtime.
    Background (grey): all instances. Highlighted (blue/red): selected.
    Timeouts at sentinel line w/ jitter. Reference lines at 1s/30s/etc.

    Args:
        data (list[dict]): All instances (background).
        selected (list[dict]): Filtered selection (highlighted).
        output_path (str): PNG output.
    """
    import matplotlib
    matplotlib.use('Agg')  # Headless
    import matplotlib.pyplot as plt
    import matplotlib.patches as mpatches
    import numpy as np

    TIMEOUT_SENTINEL = 1850.0   # Y-position for timeout markers

    # Split into groups
    selected_keys = {d['s_idigest'] for d in selected}
    bg_valid    = [d for d in data if d['s_result'] == 'Valid'   and d['s_idigest'] not in selected_keys]
    bg_timeout  = [d for d in data if d['s_result'] == 'Timeout' and d['s_idigest'] not in selected_keys]
    sel_valid   = [d for d in data if d['s_result'] == 'Valid'   and d['s_idigest'] in selected_keys]
    sel_timeout = [d for d in data if d['s_result'] == 'Timeout' and d['s_idigest'] in selected_keys]

    fig, ax = plt.subplots(figsize=(14, 8))

    def jitter_points(n: int, scale: float = 0.015) -> np.ndarray:
        """Vertical jitter for timeout markers."""
        return np.random.uniform(-scale * TIMEOUT_SENTINEL,
                                 scale * TIMEOUT_SENTINEL, n) + TIMEOUT_SENTINEL

    # Background instances (grey, faint)
    if bg_valid:
        bg_leaves = np.array([d['s_num_leaves'] for d in bg_valid])
        bg_times  = np.array([d['s_wtime'] for d in bg_valid])
        ax.scatter(bg_leaves, bg_times, c='#cccccc', s=6, alpha=0.25,
                   label=f'Valid (not selected, n={len(bg_valid)})', zorder=1)

    if bg_timeout:
        bg_to_leaves = np.array([d['s_num_leaves'] for d in bg_timeout])
        bg_to_jitter = jitter_points(len(bg_timeout))
        ax.scatter(bg_to_leaves, bg_to_jitter, c='#ffaaaa', s=6, alpha=0.25,
                   marker='x', label=f'Timeout (not selected, n={len(bg_timeout)})', zorder=1)

    # Selected instances (bold colors)
    if sel_valid:
        sel_leaves = np.array([d['s_num_leaves'] for d in sel_valid])
        sel_times  = np.array([d['s_wtime'] for d in sel_valid])
        ax.scatter(sel_leaves, sel_times, c='#1f77b4', s=35, alpha=0.85,
                   edgecolors='white', linewidth=0.5,
                   label=f'Valid selected (n={len(sel_valid)})', zorder=3)

    if sel_timeout:
        sel_to_leaves = np.array([d['s_num_leaves'] for d in sel_timeout])
        sel_to_jitter = jitter_points(len(sel_timeout))
        ax.scatter(sel_to_leaves, sel_to_jitter, c='#d62728', s=40, alpha=0.95,
                   marker='X', edgecolors='darkred', linewidth=1.0,
                   label=f'Timeout selected (n={len(sel_timeout)})', zorder=3)

    # Red timeout sentinel line
    ax.axhline(TIMEOUT_SENTINEL, color='red', linestyle='--', linewidth=1.5,
               alpha=0.7, zorder=0)
    ax.text(0.98, TIMEOUT_SENTINEL * 1.02, '30min timeout', color='red',
            fontsize=10, ha='right', va='bottom', transform=ax.transData)

    # Grey time reference lines
    for t, label in [(1, '1s'), (30, '30s'), (300, '5m'), (900, '15m')]:
        ax.axhline(t, color='grey', linestyle=':', linewidth=1.0, alpha=0.4)
        ax.text(0.02, t * 1.1, label, color='grey', fontsize=9,
                transform=ax.transData, va='center')

    # Formatting
    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlabel('Number of leaves (log scale)', fontsize=12, fontweight='bold')
    ax.set_ylabel('Prior wall-clock time (s, log scale)', fontsize=12, fontweight='bold')
    ax.set_xlim(1, 20000)
    ax.set_ylim(0.0005, TIMEOUT_SENTINEL * 1.1)

    title = (f'Stride Test Selection: {len(selected)} instances from {len(data)} total\n'
             f'Prior: {len([d for d in selected if d["s_result"]=="Valid"])} valid, '
             f'{len([d for d in selected if d["s_result"]=="Timeout"])} timeouts')
    ax.set_title(title, fontsize=13, fontweight='bold', pad=20)

    ax.grid(True, which='both', alpha=0.15, linestyle='-', linewidth=0.5)
    ax.legend(fontsize=10, loc='upper right', framealpha=0.95)

    plt.tight_layout()
    plt.savefig(output_path, dpi=200, bbox_inches='tight', facecolor='white')
    plt.close()
    print(f"Plot saved: {output_path}")

def main(summary_file: str, output_file: str) -> None:
    """
    Main workflow.
    """
    data = load_summary(summary_file)
    valid_data = [d for d in data if d['s_result'] == 'Valid']

    print_stats(data, valid_data)

    # Time sampling: ALL FAST + sampled slow
    categories = categorize_by_time(valid_data, TIME_BUCKETS)
    print("Time categories:")
    for name, cat in categories.items():
        print(f"  {name}: {len(cat)}")

    # Time sampling: SAMPLED fast (per SAMPLE_FRACS) + sampled slow
    fast_selected = sample_categories(categories, SAMPLE_FRACS)   # NOW USED
    slow_sampled  = sample_slow_categories(categories, SLOW_SAMPLE_FRACS)

    # Hard small
    hard_small = select_hard_small(data, SMALL_LEAVES_MAX, HARD_SMALL_VALID_N, HARD_SMALL_TIMEOUT_N)

    # Buckets w/ stats
    bucket_selected, bucket_stats = select_buckets(data, BUCKET_SIZES)
    print("\nBucket selection summary:")
    for b, stats in bucket_stats.items():
        print(f"  {b}: {stats['selected']}/{stats['total']} ({stats['timeouts']} timeout)")

    # Combine & dedup
    selected = dedup(fast_selected + slow_sampled + hard_small + bucket_selected)

    # Huge solved
    huge = find_huge_solved(valid_data, HUGE_LEAVES_MIN, HUGE_TIME_MAX)
    if huge:
        print("\nHuge solved (<1h, >=400 leaves):")
        for d in huge:
            print(f"  {d['s_idigest']}: {d['s_num_leaves']}l, {d['s_wtime']:.0f}s")

    write_lst(selected, output_file)
    print(f"\nSelected {len(selected)} unique -> {output_file}")
    print_selection_stats(data, selected)
    plot_selection(data, selected, output_path="selection_stats.png")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Filter Stride summary.json to .lst")
    parser.add_argument("summary", help="summary.json path")
    parser.add_argument("output", help=".lst output path")
    args = parser.parse_args()
    main(args.summary, args.output)
