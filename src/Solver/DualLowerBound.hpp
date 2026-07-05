#ifndef PACE2026_DUAL_LOWER_BOUND_HPP
#define PACE2026_DUAL_LOWER_BOUND_HPP

#include "../Graph/Instance.hpp"

namespace solver
{

/// \brief A certified lower bound L on the MAF optimum k* (in component units): every maximum
/// agreement forest has at least L components. It is derived from a feasible dual solution of an LP
/// relaxation of MAF that the 3-approximation builds as a by-product (Schalekamp, van Zuylen, van der
/// Ster, arXiv:1511.06000, Algorithm 1). By weak duality the dual objective D lower-bounds k*-1, so
/// L = D+1 satisfies L <= k* on every instance.
///
/// This is a deliberately conservative rendering of that dual: it is always valid (L <= k* — the
/// property certified early exit relies on) but can be loose on adversarial topologies, because it
/// does not track the paper's reformulated-dual z-variables (which would tighten it at the cost of a
/// far more delicate construction — see the note in DualLowerBound.cpp).
///
/// Runs on internal clones of the two forests and does not mutate \p instance.
///
/// \param instance a two-forest MAF instance (as read, before reductions). For any other shape the
///        universally-valid trivial bound L = 1 is returned.
/// \return a certified lower bound L >= 1 with L <= k*.
long computeDual3ApproxLowerBound(const graph::Instance& instance);

}  // namespace solver

#endif  // PACE2026_DUAL_LOWER_BOUND_HPP
