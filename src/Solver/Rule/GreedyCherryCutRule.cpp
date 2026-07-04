#include "GreedyCherryCutRule.hpp"

solver::GreedyCherryCutRule::GreedyCherryCutRule(
    const std::shared_ptr<graph::Instance>& instance,
    const std::shared_ptr<Context>& context,
    unsigned int aLabel,
    unsigned int cLabel) :
        AbstractRule(instance, context, false),
        aLabel(aLabel),
        cLabel(cLabel)
{}

solver::RuleReturnCode solver::GreedyCherryCutRule::apply()
{
    if (this->isApplied)
    {
        throw std::invalid_argument("GreedyCherryCutRule : apply : rule is already applied");
    }
    isApplied = true;

    // Isolate aLabel in every forest: after this, a is a singleton block everywhere, which is a
    // valid block of any agreement forest. Forests in which a is already a root are skipped.
    for (const auto& f : *instance)
    {
        graph::Node* aNode = f->LabelToTerminal().at(aLabel);
        if (aNode->parent == nullptr)
        {
            continue;
        }
        if (context->protectedEdges.contains(aNode))
        {
            return RuleReturnCode::CutBranch;
        }
        changes.emplace(aNode, f);
        changes.top().doAction();
    }

    return RuleReturnCode::Continue;
}

void solver::GreedyCherryCutRule::unapply()
{
    if (not this->isApplied)
    {
        throw std::invalid_argument("GreedyCherryCutRule : unapply : rule is not applied");
    }
    isApplied = false;

    while (not changes.empty())
    {
        changes.top().undoAction();
        changes.pop();
    }
}

std::shared_ptr<solver::AbstractRule>
solver::GreedyCherryCutRule::isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                          const std::shared_ptr<Context>& context)
{
    const auto& f0 = instance->at(0);

    // Find the first cherry (a, c) in the reference forest f0.
    unsigned int aLabel = 0;
    unsigned int cLabel = 0;
    for (const auto& [node, label] : f0->TerminalToLabel())
    {
        if (node->sibling != nullptr and f0->TerminalToLabel().contains(node->sibling))
        {
            aLabel = label;
            cLabel = f0->TerminalToLabel().at(node->sibling);
            break;
        }
    }

    if (aLabel == 0)
    {
        // No cherry in f0 — nothing to resolve here (Check/Propagation rules handle this).
        return nullptr;
    }

    // Only fire when (a, c) is NOT a common cherry: if it is common to every forest,
    // EqualPairReductionRule (higher priority) contracts it instead.
    for (unsigned int i = 1; i < instance->size(); i++)
    {
        const auto& fi = instance->at(i);
        const auto& aNode = fi->LabelToTerminal().at(aLabel);
        const auto& cNode = fi->LabelToTerminal().at(cLabel);
        if (aNode->sibling != cNode)
        {
            // Found a forest where (a, c) is not a sibling pair — the cherry conflicts, so cut.
            return std::make_shared<GreedyCherryCutRule>(instance, context, aLabel, cLabel);
        }
    }

    // (a, c) is a common cherry across all forests — defer to EqualPairReductionRule.
    return nullptr;
}

std::string solver::GreedyCherryCutRule::name() const
{
    return "GreedyCherryCutRule";
}

std::shared_ptr<solver::AbstractRule> solver::GreedyCherryCutRule::clone() const
{
    return std::make_shared<GreedyCherryCutRule>(instance, context, aLabel, cLabel);
}
