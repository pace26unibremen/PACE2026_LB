#include "ACBranchingRule.hpp"

#include <cassert>

solver::ACBranchingRule::ACBranchingRule(
    const std::shared_ptr<graph::Instance>& instance,
    const std::shared_ptr<Context>& context,
    unsigned int aLabel,
    unsigned int cLabel) :
        AbstractBranchingRule(instance, context, 2),
        aLabel(aLabel),
        cLabel(cLabel)
{}

solver::RuleReturnCode solver::ACBranchingRule::apply()
{
    if (this->isApplied)
    {
        throw std::invalid_argument("ACBranchingRule : apply : rule is already applied");
    }
    if (this->branch >= 2)
    {
        throw std::invalid_argument("ACBranchingRule : apply : all branches are already visited");
    }
    isApplied = true;
    branch++;

    switch (branch)
    {
        case 1:
            for(const auto& f : *instance)
            {
                auto aNode = f->LabelToTerminal().at(aLabel);
                if (not aNode->parent)
                    continue;
                if (context->protectedEdges.contains(aNode))
                    return RuleReturnCode::CutBranch;
                changes.emplace(aNode, f);
                changes.top().doAction();
            }
            break;
        case 2:
            for(const auto& f : *instance)
            {
                auto cNode = f->LabelToTerminal().at(cLabel);
                if (not cNode->parent)
                    continue;
                if (context->protectedEdges.contains(cNode))
                    return RuleReturnCode::CutBranch;
                changes.emplace(cNode, f);
                changes.top().doAction();

                // we can protect edges to a-Node
                // because we already tested all cases where a-Node is cut in branch 1
                const auto aNode = f->LabelToTerminal().at(aLabel);
                if (not context->protectedEdges.contains(aNode))
                {
                    edgeProtections.emplace(aNode);
                    context->protectedEdges.emplace(aNode);
                }
            }
            break;
        default:
            assert(false);
    }

    return RuleReturnCode::Continue;
}

void solver::ACBranchingRule::unapply()
{
    if (not this->isApplied)
    {
        throw std::invalid_argument("ACBranchingRule : unapply : rule is not applied");
    }
    isApplied = false;

    while (not changes.empty())
    {
        changes.top().undoAction();
        changes.pop();
    }

    if (branch == 2)
    {
        for (const auto& t : edgeProtections)
        {
            context->protectedEdges.erase(t);
        }
        edgeProtections.clear();
    }
}


std::shared_ptr<solver::AbstractRule>
solver::ACBranchingRule::isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                                   const std::shared_ptr<Context>& context)
{
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
        const auto& aNode = fi->LabelToTerminal()[aLabel];
        const auto& cNode = fi->LabelToTerminal()[cLabel];
        const auto& aRoot = fi->rootOf(aNode);
        if (not cNode->hasSubsetTerminals(aRoot))
        {
            return std::make_shared<ACBranchingRule>(instance, context, aLabel, cLabel);
        }
    }

    return nullptr;
}

std::string solver::ACBranchingRule::name() const
{
    return "ACBranchingRule";
}

std::shared_ptr<solver::AbstractRule> solver::ACBranchingRule::clone() const
{
    auto clone = std::make_shared<ACBranchingRule>(instance, context, aLabel, cLabel);
    clone->branch = isApplied ? branch - 1 : branch;
    return clone;
}
