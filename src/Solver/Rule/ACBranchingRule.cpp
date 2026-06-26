#include "ACBranchingRule.hpp"

#include <cassert>

// 1. label1
// 2. label2
// 3. list of forests that are affected
typedef std::tuple<
        unsigned int,
        unsigned int,
        std::list<std::shared_ptr<graph::Forest>>>
    affectedForests_type;

solver::ACBranchingRule::ACBranchingRule(
    const std::shared_ptr<graph::Instance>& instance,
    const std::shared_ptr<Context>& context,
    const affectedForests_type& affectedForests) :
        AbstractBranchingRule(instance, context, 2),
        label1(get<0>(affectedForests)),
        label2(get<1>(affectedForests)),
        forestsConnectedLabels(get<2>(affectedForests))
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
            for(const auto& f : forestsConnectedLabels)
            {
                auto t1 = f->LabelToTerminal()[label1];
                if (context->protectedEdges.contains(t1))
                {
                    return RuleReturnCode::CutBranch;
                }
                changes.emplace(t1, f);
                changes.top().doAction();
            }
            break;
        case 2:
            for(const auto& f : forestsConnectedLabels)
            {
                auto t2 = f->LabelToTerminal()[label2];
                if (context->protectedEdges.contains(t2))
                {
                    return RuleReturnCode::CutBranch;
                }
                changes.emplace(t2, f);
                changes.top().doAction();

                // we can protect edges to t1
                // because we already tested all cases where t1 is cut in branch 1
                const auto t1 = f->LabelToTerminal()[label1];
                if (not context->protectedEdges.contains(t1))
                {
                    edgeProtections.emplace(t1);
                    context->protectedEdges.emplace(t1);
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
    affectedForests_type af = affectedForests_type();
    get<0>(af) = 0;
    get<1>(af) = 0;

    auto f = instance->at(0);
    for (const auto& [label, node] : f->LabelToTerminal())
    {
        if (node->sibling != nullptr and f->TerminalToLabel().contains(node->sibling))
        {
            get<0>(af) = label;
            get<1>(af) = node->sibling->smallestTerminal();
            get<2>(af).push_back(f);
            break;
        }
    }

    if (get<0>(af) == 0)
    {
        // we have a better rule for this case
        return nullptr;
    }

    bool existsUnconnectedPair = false;
    for (unsigned int i = 1; i < instance->size(); i++)
    {
        auto fi = instance->at(i);
        auto t1 = fi->LabelToTerminal()[get<0>(af)];
        auto t2 = fi->LabelToTerminal()[get<1>(af)];
        auto root = fi->rootOf(t1);
        if (not t2->hasSubsetTerminals(root))
        {
            existsUnconnectedPair = true;
        }
        else
        {
            get<2>(af).push_back(fi);
        }
    }

    if (not existsUnconnectedPair)
    {
        return nullptr;
    }

    return std::make_shared<ACBranchingRule>(instance, context, af);
}

std::string solver::ACBranchingRule::name() const
{
    return "ACBranchingRule";
}

std::shared_ptr<solver::AbstractRule> solver::ACBranchingRule::clone() const
{
    affectedForests_type af = {label1, label2, forestsConnectedLabels};
    auto clone = std::make_shared<ACBranchingRule>(instance, context, af);
    clone->branch = isApplied ? branch - 1 : branch;
    return clone;
}
