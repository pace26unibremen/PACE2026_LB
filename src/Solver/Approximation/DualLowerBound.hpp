#ifndef PACE2026_DUAL_LOWER_BOUND_HPP
#define PACE2026_DUAL_LOWER_BOUND_HPP

#include "../../Graph/Instance.hpp"

#include <atomic>
#include <chrono>

namespace solver
{

/// A wall-clock deadline for the (slow) 2-approx dual. The default is "never", i.e. run to completion.
using Deadline = std::chrono::steady_clock::time_point;
inline Deadline noDeadline() { return Deadline::max(); }

/// \brief A certified lower bound L on the MAF optimum k* (in component units): every maximum
/// agreement forest has at least L components. It is derived from a feasible dual solution of an LP
/// relaxation of MAF that the 3-approximation builds as a by-product (Schalekamp, van Zuylen, van der
/// Ster, arXiv:1511.06000, Algorithm 1). By weak duality the dual objective D lower-bounds k*-1, so
/// L = D+1 satisfies L <= k* on every instance.
///
/// This is a deliberately conservative rendering of that dual: it is always valid (L <= k* — the
/// property certified early exit relies on) but can be loose on adversarial topologies, because it
/// does not track the paper's reformulated-dual z-variables (which would tighten it at the cost of a
/// far more delicate construction — see the note in RedBlueDual.cpp). Fast: O(n^2).
///
/// Runs on internal clones of the two forests and does not mutate \p instance.
///
/// \param instance a two-forest MAF instance (as read, before reductions). For any other shape the
///        universally-valid trivial bound L = 1 is returned.
/// \return a certified lower bound L >= 1 with L <= k*.
long computeDual3ApproxLowerBound(const graph::Instance& instance);

/// \brief A tighter certified lower bound L on k*, from the *2-approximation* ("Red-Blue") dual of the
/// same paper (Schalekamp, van Zuylen, van der Ster, arXiv:1511.06000, Algorithm 2 + Procedures 1-4c).
///
/// Like \ref computeDual3ApproxLowerBound it builds a feasible dual solution and emits its objective
/// L = D+1 = sum(y), so L <= k* on every instance. The Red-Blue construction credits more dual per cut
/// (it handles a whole minimal incompatible active sibling set R u B via a reformulated dual), giving a
/// bound that is empirically ~2x tighter than the 3-approx dual (mean L/k* ~0.78) and exact on ~19% of
/// instances, validated L <= k* with 0 violations on 2500+ exact-checked instances.
///
/// Only the dual value is needed, so the paper's retroactive merges (Procedures 4b/4c) -- which only
/// reduce the primal cut count and never change a y-variable -- are skipped. For instances with more
/// than two forests the max over ordered forest pairs is returned (k*(all) >= k*(pair)).
///
/// Runs on internal clones of the forests and does not mutate \p instance. For a single forest the
/// trivial bound L = 1 is returned.
///
/// \param deadline optional wall-clock cap. If the computation would run past it, it aborts and returns
///        0 (the "did not finish in time" sentinel) rather than a partial -- hence invalid -- bound.
///        With the default \ref noDeadline it always runs to completion and returns L >= 1.
/// \param stop optional cooperative-cancellation flag. When it becomes true the computation aborts and
///        returns 0, exactly like a hit deadline. Used to run this on a background thread and cancel it
///        the moment the search finishes. Checked at the top of the main loop and of preprocess.
/// \return a certified lower bound L >= 1 with L <= k*, or 0 if it was cancelled before finishing.
long computeDual2ApproxLowerBound(const graph::Instance& instance, Deadline deadline = noDeadline(),
                                  const std::atomic<bool>* stop = nullptr);

/// \brief The best certified lower bound available, staged: always the fast 3-approx dual, plus the
/// tighter 2-approx dual when it finishes within \p deadline. Both are valid (<= k*), so their maximum
/// is valid too and never worse than the 3-approx. If the 2-approx hits the deadline it is simply
/// dropped and the 3-approx bound is returned -- so the lower-bound track never wastes its time budget
/// on the expensive dual for a huge instance. This is what callers should use.
long computeCertifiedLowerBound(const graph::Instance& instance, Deadline deadline = noDeadline());

}  // namespace solver

#endif  // PACE2026_DUAL_LOWER_BOUND_HPP
