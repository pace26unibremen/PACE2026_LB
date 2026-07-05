#include "LowerBoundSolver.hpp"

#include <cmath>

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
    bool solved = false;
    int i = 1;
    while (!solved)
    {
        // branchingSolver->SetTimeout(i*30); oder so ähnlich...
        solved = branchingSolver->solve();
        maxsatSolver->setTimeOut(30.0*i);
        maxsatSolver->solve();
        int maxsatLB = static_cast<int> (std::floor((context->a * maxsatSolver->getCurrentLowerBound()) + context->b));


    }
    // Funktionscalls für maxsatSolver:
    // maxsatSolver->solve();
    // maxsatSolver->setTimeOut(double second);
    // maxsatSolver->getCurrentLowerBound();
}
} // namespace solver