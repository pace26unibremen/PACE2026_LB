#include "LowerBoundSolver.hpp"

namespace solver {

// Constructor 
LowerBoundSolver::LowerBoundSolver (const std::shared_ptr<graph::Instance>& instance,
                                    const std::shared_ptr<BranchingSolver>& branchingSolver) 
            : AbstractSolver(instance),
            branchingSolver(branchingSolver),
            context(branchingSolver->GetContext())
{
    maxsatSolver = std::make_shared<solver::IncrementalMAFSolver>(instance);
}

bool LowerBoundSolver::solve()
{
    // TODO
    // Funktionscalls für maxsatSolver:
    // maxsatSolver->solve();
    // maxsatSolver->setTimeOut(double second);
    // maxsatSolver->getCurrentLowerBound();
}
} // namespace solver