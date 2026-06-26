#!/bin/bash
# check_errors.sh - Check for solver errors after a benchmark run
# Usage: check_errors.sh <local_stride_logs_dir>
#   local_stride_logs_dir: path to the rsync'd stride-logs directory for this job
#                          (contains exactly one run subdirectory + a 'latest' symlink)

LOCAL_LOGS="${1:?Usage: check_errors.sh <local_stride_logs_dir>}"

# Find the single run directory (follow the latest symlink if present,
# otherwise just pick the only subdirectory).
if [ -L "${LOCAL_LOGS}/latest" ]; then
    RUN_DIR_NAME=$(basename "$(readlink -f "${LOCAL_LOGS}/latest")")
else
    # Fallback: only one real subdir should exist
    RUN_DIR_NAME=$(find "$LOCAL_LOGS" -mindepth 1 -maxdepth 1 -type d | head -n1 | xargs basename)
fi

if [ -z "$RUN_DIR_NAME" ]; then
    echo "Warning: Could not determine run directory for error checking"
    exit 1
fi

LOCAL_ERROR_DIR="${LOCAL_LOGS}/${RUN_DIR_NAME}/solvererror"

if [ -d "$LOCAL_ERROR_DIR" ] && [ -n "$(ls -A "$LOCAL_ERROR_DIR")" ]; then
    echo "Error: solvererror non-empty in $RUN_DIR_NAME"
    exit 1
fi

echo "No solver errors found in $RUN_DIR_NAME"

# stride drops every instance whose result didn't match the known best score into 'valid'
# (despite being a structurally valid forest). Since the solver now reports its best solution
# on SIGTERM, instances cut off by the time limit ("#s timeout 1" in stdout) legitimately end
# up here too and are not a bug. Only flag entries that are suboptimal *without* having been
# cut off — those indicate the solver gave up early or has a correctness issue.
LOCAL_VALID_DIR="${LOCAL_LOGS}/${RUN_DIR_NAME}/valid"
SUBOPTIMAL_WITHOUT_TIMEOUT=()

if [ -d "$LOCAL_VALID_DIR" ]; then
    for instance_dir in "$LOCAL_VALID_DIR"/*/; do
        [ -d "$instance_dir" ] || continue
        STDOUT_FILE="${instance_dir}stdout"
        [ -f "$STDOUT_FILE" ] || continue

        TIMEOUT_FLAG=$(grep -m1 '^#s timeout' "$STDOUT_FILE" | awk '{print $3}')
        if [ "$TIMEOUT_FLAG" != "1" ]; then
            SUBOPTIMAL_WITHOUT_TIMEOUT+=("$(basename "$instance_dir")")
        fi
    done
fi

if [ "${#SUBOPTIMAL_WITHOUT_TIMEOUT[@]}" -gt 0 ]; then
    echo "Error: ${#SUBOPTIMAL_WITHOUT_TIMEOUT[@]} suboptimal solve(s) in $RUN_DIR_NAME without a SIGTERM cutoff:"
    printf '  %s\n' "${SUBOPTIMAL_WITHOUT_TIMEOUT[@]}"
    exit 1
fi

echo "No suboptimal solves without a SIGTERM cutoff found in $RUN_DIR_NAME"