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
    /// \param initialLowerBound a certified lower bound L <= k* already known before the coordinator
    ///        runs (e.g. the 2-/3-approx dual computed in startSolver). It seeds the coordinator's L
    ///        floor so the incremental SAT — which is typically looser than the dual — can never lower
    ///        the certified threshold below what the dual already proved.
    LowerBoundSolver(const std::shared_ptr<graph::Instance>& instance,
                     const std::shared_ptr<BranchingSolver>& branchingSolver,
                     const std::shared_ptr<IIncrementalLowerBound>& lowerBoundSolver,
                     LowerBoundCoordinatorConfig config = LowerBoundCoordinatorConfig::defaults(),
                     long initialLowerBound = 0);

    ~LowerBoundSolver() override = default;

    bool solve() override;

  private:
    std::shared_ptr<BranchingSolver> branchingSolver;
    std::shared_ptr<IIncrementalLowerBound> lowerBoundSolver;
    std::shared_ptr<Context> context;
    LowerBoundCoordinatorConfig config;
    /// \brief A certified lower bound known before the run (dual bound); the coordinator's L never
    /// drops below this, so a looser incremental SAT bound cannot lower the certified threshold.
    long initialLowerBound;

    /// \brief Adopt L into the certified threshold and report whether the incumbent now certifies.
    [[nodiscard]] bool adoptLowerBound(int candidate, int& currentL);
    /// \brief True when the incumbent size is within the current certified threshold.
    [[nodiscard]] bool incumbentCertifies() const;
};

}  // namespace solver

#endif  // PACE2026_LOWERBOUND_SOLVER_HPP
