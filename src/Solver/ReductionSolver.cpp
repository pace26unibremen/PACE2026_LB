#include "ReductionSolver.hpp"

solver::ReductionSolver::ReductionSolver(const std::shared_ptr<graph::Instance>& instance) :
        AbstractSolver(instance)
{}

bool solver::ReductionSolver::solve()
{
    subtreeReductionRule = solver::SubtreeReductionRule::isApplicable(instance, context);
    if (subtreeReductionRule)
    {
        subtreeReductionRule->apply();
    }
    return false;
}

void solver::ReductionSolver::unapplyReductions()
{
    if (subtreeReductionRule)
    {
        subtreeReductionRule->unapply();
    }
}
