#include "ForcedCherryCutRule.hpp"

solver::ForcedCherryCutRule::ForcedCherryCutRule(
    const std::shared_ptr<graph::Instance>& instance,
    const std::shared_ptr<Context>& context,
    unsigned int aLabel,
    unsigned int cLabel) :
        AbstractRule(instance, context, false),
        aLabel(aLabel),
        cLabel(cLabel)
{}

solver::RuleReturnCode solver::ForcedCherryCutRule::apply()
{
    if (this->isApplied)
    {
        throw std::invalid_argument("ForcedCherryCutRule : apply : rule is already applied");
    }
    isApplied = true;

    const auto& t2 = instance->at(1);
    auto aNode = t2->LabelToTerminal().at(aLabel);
    auto cNode = t2->LabelToTerminal().at(cLabel);
    auto bNode = aNode->sibling;

    if (context->protectedEdges.contains(aNode)
        or context->protectedEdges.contains(bNode)
        or context->protectedEdges.contains(cNode))
    {
        return RuleReturnCode::CutBranch;
    }

    if (aNode->parent)
    {
        changes.emplace(aNode, t2);
        changes.top().doAction();
    }

    if (bNode->parent)
    {
        changes.emplace(bNode, t2);
        changes.top().doAction();
    }

    if (cNode->parent)
    {
        changes.emplace(cNode, t2);
        changes.top().doAction();
    }

    return RuleReturnCode::Continue;
}

void solver::ForcedCherryCutRule::unapply()
{
    if (not this->isApplied)
    {
        throw std::invalid_argument("ForcedCherryCutRule : unapply : rule is not applied");
    }
    isApplied = false;

    while (not changes.empty())
    {
        changes.top().undoAction();
        changes.pop();
    }
}

std::shared_ptr<solver::AbstractRule>
solver::ForcedCherryCutRule::isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                          const std::shared_ptr<Context>& context)
{
    if (instance->size() != 2)
    {
        return nullptr;
    }

    unsigned int aLabel = 0;
    unsigned int cLabel = 0;

    const auto& t1 = instance->at(0);
    for (const auto& [node, label] : t1->TerminalToLabel())
    {
        if (node->sibling != nullptr and t1->TerminalToLabel().contains(node->sibling))
        {
            aLabel = label;
            cLabel = t1->TerminalToLabel().at(node->sibling);
            break;
        }
    }

    if (aLabel == 0)
    {
        // no sibling pair in T1 — nothing for this rule to do
        return nullptr;
    }

    const auto& t2 = instance->at(1);
    const auto& aNode = t2->LabelToTerminal().at(aLabel);
    const auto& cNode = t2->LabelToTerminal().at(cLabel);

    if (aNode->sibling == cNode)
    {
        // (a,c) is also a sibling pair in T2 — EqualPairReductionRule's job, not ours
        return nullptr;
    }

    if (aNode->parent == nullptr or cNode->parent == nullptr)
    {
        // a or c is already a singleton in T2 — SingleVertexTreePropagationRule's job
        return nullptr;
    }

    return std::make_shared<ForcedCherryCutRule>(instance, context, aLabel, cLabel);
}

std::string solver::ForcedCherryCutRule::name() const
{
    return "ForcedCherryCutRule";
}

std::shared_ptr<solver::AbstractRule> solver::ForcedCherryCutRule::clone() const
{
    return std::make_shared<ForcedCherryCutRule>(instance, context, aLabel, cLabel);
}
