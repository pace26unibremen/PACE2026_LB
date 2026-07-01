#include "EqualPairReductionRule.hpp"

#include <cassert>

solver::EqualPairReductionRule::EqualPairReductionRule(const std::shared_ptr<graph::Instance>& instance,
                                     const std::shared_ptr<Context>& context,
                                     unsigned int aLabel,
                                     unsigned int cLabel) :
    AbstractRule(instance,context, true),
    aLabel(aLabel),
    cLabel(cLabel)
{}

solver::RuleReturnCode solver::EqualPairReductionRule::apply()
{
    if (this->isApplied)
    {
        throw std::invalid_argument("EqualPairReductionRule : apply : rule is already applied");
    }
    isApplied = true;

    for(const auto& f : *instance)
    {
        const auto& siblingRoot = f->LabelToTerminal().at(aLabel)->parent;
        changes.emplace(siblingRoot, f);
        changes.top().doAction();
    }

    return RuleReturnCode::Continue;
}

void solver::EqualPairReductionRule::unapply()
{
    if (not this->isApplied)
    {
        throw std::invalid_argument("EqualPairReductionRule : unapply : rule is not applied");
    }
    isApplied = false;

    while (not changes.empty())
    {
        changes.top().undoAction();
        changes.pop();
    }
}


std::shared_ptr<solver::AbstractRule>
solver::EqualPairReductionRule::isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                    const std::shared_ptr<Context>& context)
{
    auto forestToSubtree = std::unordered_map<std::shared_ptr<graph::Forest>, graph::Node*>();

    unsigned int aLabel = 0;
    unsigned int cLabel = 0;

    const auto& f0 = instance->at(0);
    for (const auto& [node, label] : f0->TerminalToLabel())
    {
        if (node->sibling != nullptr and f0->TerminalToLabel().contains(node->sibling))
        {
            aLabel = label;
            cLabel = f0->TerminalToLabel().at(node->sibling);
            break;
        }
    }

    // check if we found a sibling pair in f0
    if (aLabel == 0)
    {
        // we have a better rule for this case
        // (CheckSingleVertexTreesRule, SingleVertexTreePropagationRule, EqualForestRule)
        return nullptr;
    }

    for (unsigned int i = 1; i < instance->size(); i++)
    {
        const auto& fi = instance->at(i);
        const auto& aNode = fi->LabelToTerminal().at(aLabel);
        const auto& cNode = fi->LabelToTerminal().at(cLabel);
        if (aNode->sibling != cNode)
        {
            // we can not apply this rule for aLabel, cLabel
            return nullptr;
        }
    }

    return std::make_shared<EqualPairReductionRule>(instance, context, aLabel, cLabel);
}

std::string solver::EqualPairReductionRule::name() const
{
    return "EqualPairReductionRule";
}

std::shared_ptr<solver::AbstractRule> solver::EqualPairReductionRule::clone() const
{
    return std::make_shared<EqualPairReductionRule>(instance, context, aLabel, cLabel);
}
