#include "EqualPairReductionRule.hpp"

#include <cassert>

solver::EqualPairReductionRule::EqualPairReductionRule(const std::shared_ptr<graph::Instance>& instance,
                                     const std::shared_ptr<Context>& context,
                                     const std::unordered_map<std::shared_ptr<graph::Forest>,
                                     graph::Node*>& forestToSubtree) :
    AbstractRule(instance,context, true),
    forestToSubtree(forestToSubtree)
{
    this->changes = std::stack<solver::CollapseSubtreeAction>();
}


solver::RuleReturnCode solver::EqualPairReductionRule::apply()
{
    if (this->isApplied)
    {
        throw std::invalid_argument("EqualPairReductionRule : apply : rule is already applied");
    }
    isApplied = true;

    for(const auto& [f,subtree] : forestToSubtree)
    {
        changes.emplace(subtree, f);
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

    unsigned int label1 = 0;
    unsigned int label2 = 0;

    auto f = instance->at(0);
    for (const auto& [label, node] : f->LabelToTerminal())
    {
        if (node->sibling != nullptr and f->TerminalToLabel().contains(node->sibling))
        {
            label1 = label;
            label2 = node->sibling->smallestTerminal();
            forestToSubtree.emplace(f, node->parent);
            break;
        }
    }

    if (label1 == 0)
    {
        // we have a better rule for this case
        return nullptr;
    }

    for (unsigned int i = 1; i < instance->size(); i++)
    {
        auto fi = instance->at(i);
        auto t1 = fi->LabelToTerminal()[label1];
        auto t2 = fi->LabelToTerminal()[label2];
        if ((t1->parent == nullptr) or (t1->parent != t2->parent))
        {
            // we can not apply this rule for label1, label2
            return nullptr;
        }
        forestToSubtree.emplace(fi, t1->parent);
    }

    return std::make_shared<EqualPairReductionRule>(instance, context, forestToSubtree);
}

std::string solver::EqualPairReductionRule::name() const
{
    return "EqualPairReductionRule";
}

std::shared_ptr<solver::AbstractRule> solver::EqualPairReductionRule::clone() const
{
    return std::make_shared<EqualPairReductionRule>(instance, context, forestToSubtree);
}
