#ifndef PACE2026_LOWERBOUND_SOLVER_HPP
#define PACE2026_LOWERBOUND_SOLVER_HPP

#include "AbstractSolver.hpp"
#include "BranchingSolver.hpp"
#include "Context.hpp"
#include "LowerBoundCoordinatorConfig.hpp"
#include "../Graph/Instance.hpp"

#include <memory>

namespace solver
{

/// \brief Minimal seam over an incremental lower-bound solver, so the coordinator is testable without
/// the IPAMIR/UWrMaxSat backend. \ref IncrementalMAFSolver satisfies this via a thin adapter.
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
