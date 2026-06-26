#include "CutBranchRule.hpp"

#include "../BranchingSolverConfiguration.hpp"

#include <algorithm>
#include <ranges>

solver::CutBranchRule::CutBranchRule(const std::shared_ptr<graph::Instance>& instance,
                                     const std::shared_ptr<Context>& context) :
    AbstractRule(instance, context, false)
{}

solver::RuleReturnCode solver::CutBranchRule::apply()
{
    if (this->isApplied)
    {
        throw std::invalid_argument("CutBranchRule : apply : rule is already applied");
    }
    isApplied = true;
    return RuleReturnCode::CutBranch;
}

void solver::CutBranchRule::unapply()
{
    if (not this->isApplied)
    {
        throw std::invalid_argument("CutBranchRule : unapply : rule is not applied");
    }
    isApplied = false;
}

std::shared_ptr<solver::AbstractRule>
solver::CutBranchRule::isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                    const std::shared_ptr<Context>& context)
{

    if (context->branchingSolverConfiguration->boundedDephtSearch)
    {
        for (const auto& f : *instance)
        {
            if (f->Roots().size() >  context->maxSolutionSize)
            {
                return std::make_shared<solver::CutBranchRule>(instance, context);
            }
        }
    }
    else
    {
        for (const auto& f : *instance)
        {
            if (context->weightFunction(f) >= context->bestSolutionWeight)
            {
                return std::make_shared<solver::CutBranchRule>(instance, context);
            }
        }
    }
    return nullptr;
}

std::string solver::CutBranchRule::name() const
{
    return "CutBranchRule";
}

std::shared_ptr<solver::AbstractRule> solver::CutBranchRule::clone() const
{
    return std::make_shared<solver::CutBranchRule>(instance, context);
}
