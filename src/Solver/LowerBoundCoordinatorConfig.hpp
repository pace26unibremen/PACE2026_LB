#ifndef PACE2026_LOWERBOUND_COORDINATOR_CONFIG_HPP
#define PACE2026_LOWERBOUND_COORDINATOR_CONFIG_HPP

#include <vector>

namespace solver
{

/// \brief Which sub-solver runs during a coordinator time slice.
enum class CoordinatorActor
{
    Branch,  ///< The branching solver (shrinks the incumbent upper bound U).
    Sat,     ///< The incremental MaxSAT solver (grows the lower bound L).
};

/// \brief One time slice of the coordinator schedule.
struct CoordinatorSlice
{
    CoordinatorActor actor;
    double seconds;
};

/// \brief Tuning knobs for \ref LowerBoundSolver. The default schedule escalates slice length so
/// early slices probe cheaply and later ones commit more time, as agreed for the lower-bound track.
struct LowerBoundCoordinatorConfig
{
    /// \brief Ordered slices. Any budget left after the last slice is handed to the branching solver.
    std::vector<CoordinatorSlice> schedule;

    /// \brief Total wall-clock budget for the whole coordinator run (seconds). Below the 10 min limit.
    double totalBudgetSeconds = 570.0;

    /// \brief Reserve before the hard limit for finalize() + output I/O (seconds).
    double safetyMarginSeconds = 15.0;

    /// \brief The agreed default: branch 30, sat 30, branch 60, sat 60, branch 90, sat 90, then
    /// the remaining budget to a final branch slice.
    static LowerBoundCoordinatorConfig defaults()
    {
        return LowerBoundCoordinatorConfig{
            {
                {CoordinatorActor::Branch, 30.0},
                {CoordinatorActor::Sat, 30.0},
                {CoordinatorActor::Branch, 60.0},
                {CoordinatorActor::Sat, 60.0},
                {CoordinatorActor::Branch, 90.0},
                {CoordinatorActor::Sat, 90.0},
            },
            570.0,
            15.0,
        };
    }
};

}  // namespace solver

#endif  // PACE2026_LOWERBOUND_COORDINATOR_CONFIG_HPP
