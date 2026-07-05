#ifndef PACE2026_IINCREMENTAL_LOWER_BOUND_HPP
#define PACE2026_IINCREMENTAL_LOWER_BOUND_HPP

namespace solver
{

/// \brief Minimal seam over an incremental lower-bound solver, so \ref LowerBoundSolver is testable
/// without the IPAMIR/UWrMaxSat backend. \ref IncrementalMAFSolver satisfies this via a thin adapter.
///
/// Lives in its own header (rather than inside LowerBoundSolver.hpp) so that IncrementalMAFSolver.hpp
/// can implement it without dragging in the whole coordinator/branching-solver header chain.
struct IIncrementalLowerBound
{
    virtual ~IIncrementalLowerBound() = default;
    /// \brief Budget (seconds) for the next \ref solve() call.
    virtual void setTimeOut(double seconds) = 0;
    /// \brief Run one incremental slice. \return true iff the exact optimum was proven.
    virtual bool solve() = 0;
    /// \brief The best (largest) lower bound proven so far, in component units.
    virtual int getCurrentLowerBound() = 0;
};

}  // namespace solver

#endif  // PACE2026_IINCREMENTAL_LOWER_BOUND_HPP
