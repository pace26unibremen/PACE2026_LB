#ifndef PACE2026_LOWERBOUND_SOLVER_HPP
#define PACE2026_LOWERBOUND_SOLVER_HPP

#include "AbstractSolver.hpp"
#include "BranchingSolver.hpp"
#include "Context.hpp"
#include "IIncrementalLowerBound.hpp"
#include "LowerBoundCoordinatorConfig.hpp"
#include "../Graph/Instance.hpp"

#include <memory>

namespace solver
{

/// \brief Coordinator (not a real solver): time-slices the branching solver (upper bound) against an
/// incremental lower-bound solver, stopping when the incumbent certifies against floor(a*L)+b.
class LowerBoundSolver : public AbstractSolver
{
  public:
    LowerBoundSolver(const std::shared_ptr<graph::Instance>& instance,
                     const std::shared_ptr<BranchingSolver>& branchingSolver,
                     const std::shared_ptr<IIncrementalLowerBound>& lowerBoundSolver,
                     LowerBoundCoordinatorConfig config = LowerBoundCoordinatorConfig::defaults());

    ~LowerBoundSolver() override = default;

    bool solve() override;

  private:
    std::shared_ptr<BranchingSolver> branchingSolver;
    std::shared_ptr<IIncrementalLowerBound> lowerBoundSolver;
    std::shared_ptr<Context> context;
    LowerBoundCoordinatorConfig config;

    /// \brief Adopt L into the certified threshold and report whether the incumbent now certifies.
    [[nodiscard]] bool adoptLowerBound(int candidate, int& currentL);
    /// \brief True when the incumbent size is within the current certified threshold.
    [[nodiscard]] bool incumbentCertifies() const;
};

}  // namespace solver

#endif  // PACE2026_LOWERBOUND_SOLVER_HPP
