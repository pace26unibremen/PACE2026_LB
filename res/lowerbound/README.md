# Lower-bound track regression instances

These instances guard the validity of the Red-Blue (2-approximation) dual lower bound used by the
lower-bound track. On 10 August 2026 the PACE 2026 organisers reported that our solver emitted a
solution of size 7 on an instance whose optimum is 6, and a solution of size 9 where 8 was the
largest valid answer. `redblue_unsound01.nw` and `redblue_unsound02.nw` are the two instances they
sent us. The remaining files were found afterwards by mutating those two and keeping every mutation
that reproduced the same failure.

`expected.tsv` lists, per instance, the true optimum k* and the value the dual returned before the
fix. Every entry has k* = 6 and a pre-fix dual value of 7.

## What went wrong

The lower-bound track may stop the search early once the incumbent solution is small enough to be
provably valid. With a lower bound L on the optimum k*, any solution of size at most
floor(a*L) + b is also at most floor(a*k\*) + b, so it satisfies the track's validity rule. That
argument needs L <= k*. If the bound is too large, the acceptance threshold is too large as well,
and the search stops on a solution that is not good enough.

Our Red-Blue dual could return L = k* + 1. The bound is emitted as L = D + 1, where D = sum(y) - 1
is the dual objective and y are the dual variables the construction assigns while it takes the two
input trees apart. Each component the construction commits to gets one unit of dual, and various
steps subtract corrections. The final surviving component must not get a unit, because the "+1" in
L = D + 1 already accounts for it. Our main loop credited it anyway, on both of the paths that end
the loop while leaves are still active: the path where a single active leaf is left, and the path
where all remaining active leaves are mutually compatible and therefore form one component.

We had overlooked that the paper states this restriction in passing rather than as a separate rule.
Its preprocessing step finalises a leaf that is alone in its tree in the second forest, and gives it
one unit of dual, but only when that leaf "is not the last active leaf". That parenthetical is the
only place the constraint appears, and it is what keeps the last component from being counted twice.

## Why the fix is two lines

Nothing about the construction is wrong apart from those two credits, so the fix is to delete them.
The rest of the accounting, including all the corrections, is unchanged.

## Why it was not caught earlier

An over-count of one only breaks L <= k* when the dual is exact anyway. Whenever the bound is loose,
which is most of the time, the extra unit is absorbed and the result still satisfies L <= k*. That
is why the bound passed thousands of validation instances. It also means the failure appears exactly
on the instances where the bound is tight, which are the ones where it is most useful, and which a
correctness test that searches for counterexamples will find quickly.

After the fix the dual returns exactly k* on all of these instances, so they are pinned as equalities
rather than inequalities. Anything that reintroduces the extra credit turns them into 7 > 6 and fails
the test.

## How the optima were established

Two independent ways. First, the exact track. Second, brute-force enumeration of all set partitions
of the label set, accepting a partition when every block induces the same topology in both input
trees and the blocks embed into both trees without sharing a node. The two agree on every instance
here.
