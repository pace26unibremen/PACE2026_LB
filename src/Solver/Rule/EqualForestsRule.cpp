#include "EqualForestsRule.hpp"

#include <algorithm>
#include <unordered_set>

solver::EqualForestsRule::EqualForestsRule(const std::shared_ptr<graph::Instance>& instance,
                                           const std::shared_ptr<Context>& context,
                                           const std::unordered_set<std::shared_ptr<graph::Forest>>& toBeRemoved) :
    AbstractRule(instance,context, false),
    toBeRemoved(toBeRemoved)
{}

solver::RuleReturnCode solver::EqualForestsRule::apply()
{
    if (this->isApplied)
    {
        throw std::invalid_argument("EqualForestsRule : apply : rule is already applied");
    }
    isApplied = true;
    instanceBackUp = *instance;
    std::erase_if(*instance, [&](const std::shared_ptr<graph::Forest>& f) { return toBeRemoved.contains(f); });

    // if only one forest left, we have a solution candidate else we have to continue
    return instance->size() <= 1 ? RuleReturnCode::EndBranchWithSolutionCandidate : RuleReturnCode::Continue;
}

void solver::EqualForestsRule::unapply()
{
    if (not this->isApplied)
    {
        throw std::invalid_argument("EqualForestsRule : unapply : rule is not applied");
    }
    isApplied = false;
    *instance = instanceBackUp;
}


std::shared_ptr<solver::AbstractRule>
solver::EqualForestsRule::isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                       const std::shared_ptr<Context>& context)
{
    auto toBeRemoved = std::unordered_set<std::shared_ptr<graph::Forest>>();
    for (unsigned int i = 0; i < instance->size() - 1; i++)
    {
        if (toBeRemoved.contains(instance->at(i)))
        {
            continue;
        }
        for (unsigned int j = i + 1; j < instance->size(); j++)
        {
            if (*instance->at(i) == *instance->at(j))
            {
                toBeRemoved.insert(instance->at(j));
            }
        }
    }
    if(toBeRemoved.empty())
    {
        return nullptr;
    }
    return std::make_shared<EqualForestsRule>(instance, context, toBeRemoved);
}

std::string solver::EqualForestsRule::name() const
{
    return "EqualForestsRule";
}

std::shared_ptr<solver::AbstractRule> solver::EqualForestsRule::clone() const
{
    return std::make_shared<EqualForestsRule>(instance, context, toBeRemoved);
}
