#include "ReductionSolver.hpp"

#include "Rule/SubtreeReductionRule.hpp"
#include "Rule/ChainReductionRule.hpp"

solver::ReductionSolver::ReductionSolver(const std::shared_ptr<graph::Instance>& instance) :
        AbstractSolver(instance)
{}

bool solver::ReductionSolver::solve()
{
    bool progress = true;
    while (progress)
    {
        progress = false;

        if (auto rule = solver::SubtreeReductionRule::isApplicable(instance, context))
        {
            rule->apply();
            appliedRules.push_back(rule);
            progress = true;
        }

        if (auto rule = solver::ChainReductionRule::isApplicable(instance, context))
        {
            rule->apply();
            appliedRules.push_back(rule);
            progress = true;
        }
    }

    return false;
}

void solver::ReductionSolver::unapplyReductions()
{
    for (auto it = appliedRules.rbegin(); it != appliedRules.rend(); ++it)
        (*it)->unapply();
    appliedRules.clear();
}
