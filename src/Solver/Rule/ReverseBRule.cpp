#include "ReverseBRule.hpp"

solver::ReverseBRule::ReverseBRule(const std::shared_ptr<graph::Instance>& instance,
                                     const std::shared_ptr<Context>& context,
                                     const unsigned int& toCutLabel) :
    AbstractRule(instance, context, false),
    toCutLabel(toCutLabel)
{}

solver::RuleReturnCode solver::ReverseBRule::apply()
{
    if (this->isApplied)
    {
        throw std::invalid_argument("ReverseBRule : apply : rule is already applied");
    }
    isApplied = true;

    for (const auto& f : *instance)
    {
        const auto toCutNode = f->LabelToTerminal().at(toCutLabel);
        if (not toCutNode->parent)
            continue;
        if (context->protectedEdges.contains(toCutNode))
            return RuleReturnCode::CutBranch;
        changes.emplace(toCutNode, f);
        changes.top().doAction();
    }

    return RuleReturnCode::Continue;
}

void solver::ReverseBRule::unapply()
{
    if (not this->isApplied)
    {
        throw std::invalid_argument("ReverseBRule : unapply : rule is not applied");
    }
    isApplied = false;

    while (not changes.empty())
    {
        changes.top().undoAction();
        changes.pop();
    }
}

std::shared_ptr<solver::AbstractRule>
solver::ReverseBRule::isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                    const std::shared_ptr<Context>& context)
{
    unsigned int labelToCut;
    auto f1 = instance->at(0);
    unsigned int n1Label, n2Label, uncLabel;

    for (const auto& [label, node] : f1->LabelToTerminal())
    {
        if (node->sibling != nullptr and f1->TerminalToLabel().contains(node->sibling))
        {
            if (node->parent->parent != nullptr and f1->TerminalToLabel().contains(node->parent->sibling))
            {
                n1Label = f1->TerminalToLabel().at(node);
                n2Label = f1->TerminalToLabel().at(node->sibling);
                uncLabel = f1->TerminalToLabel().at(node->parent->sibling);
                bool cutN1 = true;
                bool cutN2 = true;

                for (unsigned int i = 1; i < instance->size(); ++i)
                {
                    if (not (cutN1 or cutN2))
                    {
                        break;
                    }
                    auto fi = instance->at(i);
                    graph::Node* n1 = fi->LabelToTerminal().at(n1Label);
                    graph::Node* n2 = fi->LabelToTerminal().at(n2Label);
                    graph::Node* unc = fi->LabelToTerminal().at(uncLabel);

                    if (unc->sibling == n1)
                    {
                        cutN1 = false;
                    }
                    else if (unc->sibling == n2)
                    {
                        cutN2 = false;
                    }
                    else // unc's sibling in fi is neither n1 nor n2
                    {
                        cutN1 = false;
                        cutN2 = false;
                    }
                }

                if (cutN1)
                {
                    return std::make_shared<ReverseBRule>(instance, context, n1Label);
                }
                if (cutN2)
                {
                    return std::make_shared<ReverseBRule>(instance, context, n2Label);
                }
            }
        }
    }


    return nullptr;
}

std::string solver::ReverseBRule::name() const
{
    return "ReverseBRule";
}

std::shared_ptr<solver::AbstractRule> solver::ReverseBRule::clone() const
{
    return std::make_shared<ReverseBRule>(instance, context, toCutLabel);
}
