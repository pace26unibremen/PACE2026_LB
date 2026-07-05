#include "LowerBoundSolver.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace solver
{

LowerBoundSolver::LowerBoundSolver(const std::shared_ptr<graph::Instance>& instance,
                                   const std::shared_ptr<BranchingSolver>& branchingSolver,
                                   const std::shared_ptr<IIncrementalLowerBound>& lowerBoundSolver,
                                   LowerBoundCoordinatorConfig config)
    : AbstractSolver(instance),
      branchingSolver(branchingSolver),
      lowerBoundSolver(lowerBoundSolver),
      context(branchingSolver->GetContext()),
      config(std::move(config))
{
}

bool LowerBoundSolver::incumbentCertifies() const
{
    return not std::isinf(context->bestSolutionWeight)
        && context->bestSolutionWeight <= static_cast<float>(context->certifiedThreshold);
}

bool LowerBoundSolver::adoptLowerBound(int candidate, int& currentL)
{
    if (candidate > currentL)
    {
        currentL = candidate;
        context->certifiedThreshold = context->certifiedCeiling(currentL);
    }
    return incumbentCertifies();
}

bool LowerBoundSolver::solve()
{
    using clock = std::chrono::steady_clock;
    const auto start = clock::now();
    const auto hardStop = start
        + std::chrono::duration_cast<clock::duration>(
              std::chrono::duration<double>(config.totalBudgetSeconds - config.safetyMarginSeconds));

    int currentL = 0;
    // Seed the threshold from whatever the SAT already knows (0 if fresh) so it is never left at -1.
    // The incumbent is always still infinite here, so the return value is necessarily false.
    static_cast<void>(adoptLowerBound(lowerBoundSolver->getCurrentLowerBound(), currentL));

    auto remaining = [&]() { return std::chrono::duration<double>(hardStop - clock::now()).count(); };
    auto sliceEnd = [&](double seconds)
    {
        const double capped = std::min(seconds, std::max(0.0, remaining()));
        return clock::now() + std::chrono::duration_cast<clock::duration>(
                                  std::chrono::duration<double>(capped));
    };

    // Walk the schedule; each Branch slice may Solve/Exhaust (done) or Pause (continue).
    auto runBranchSlice = [&](double seconds) -> bool
    {
        branchingSolver->setPauseDeadline(sliceEnd(seconds));
        const auto result = branchingSolver->advanceSearch();
        return result == BranchingSolver::RunResult::Solved
            || result == BranchingSolver::RunResult::Exhausted;
    };
    auto runSatSlice = [&](double seconds) -> bool
    {
        lowerBoundSolver->setTimeOut(std::max(0.0, std::min(seconds, remaining())));
        lowerBoundSolver->solve();
        return adoptLowerBound(lowerBoundSolver->getCurrentLowerBound(), currentL);
    };

    for (const auto& slice : config.schedule)
    {
        if (remaining() <= 0.0) break;

        if (slice.actor == CoordinatorActor::Branch)
        {
            // A Branch slice that returns Solved OR Exhausted is done, full stop: an exhausted
            // search has found the TRUE OPTIMUM (always a valid answer to emit on the lower-bound
            // track), and a Solved slice is either certified or the seeded solution — either way
            // there is nothing more useful to do than emit it.
            if (runBranchSlice(slice.seconds))
            {
                branchingSolver->finalize();
                return true;
            }
        }
        else
        {
            // A Sat slice only "finishes" the coordinator if raising the threshold makes the
            // current incumbent certify.
            if (runSatSlice(slice.seconds))
            {
                branchingSolver->finalize();
                return true;
            }
        }
    }

    // Remaining budget: keep branching until the clock runs out or the search finishes.
    while (remaining() > 0.0)
    {
        if (runBranchSlice(remaining()))
        {
            break;
        }
    }

    branchingSolver->finalize();
    // The lower-bound track always emits the best incumbent found, whether or not it certified.
    return not std::isinf(context->bestSolutionWeight);
}

}  // namespace solver
