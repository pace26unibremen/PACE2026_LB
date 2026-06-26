#include "ABCBranchingRule.hpp"
#include "EqualPairReductionRule.hpp"

#include <cassert>
#include <list>

// 1. label1
// 2. label2
// 3. map shared_ptr forest to the list of the cut-away subtrees on the path
typedef std::tuple<
            unsigned int,
            unsigned int,
            std::unordered_map<std::shared_ptr<graph::Forest>, std::list<graph::Node*>>>
    cuts_type;

solver::ABCBranchingRule::ABCBranchingRule(const std::shared_ptr<graph::Instance>& instance,
                                                     const std::shared_ptr<Context>& context,
                                                     const cuts_type& cuts) :
        AbstractBranchingRule(instance, context, 3),
        label1(get<0>(cuts)),
        label2(get<1>(cuts)),
        forestToPathDeletions(get<2>(cuts))
{}

solver::RuleReturnCode solver::ABCBranchingRule::apply()
{
    if (this->isApplied)
    {
        throw std::invalid_argument("ABCBranchingRule : apply : rule is already applied");
    }
    if (this->branch >= 3)
    {
        throw std::invalid_argument("ABCBranchingRule : apply : all branches are already visited");
    }
    isApplied = true;
    branch++;

    switch (branch)
    {
        case 3:
        {
            for (const auto& f : *instance)
            {
                const auto t1 = f->LabelToTerminal()[label1];
                if (t1->parent != nullptr)
                {
                    changes.emplace(t1, f);
                    changes.top().doAction();
                }
                if (context->protectedEdges.contains(t1))
                {
                    return RuleReturnCode::CutBranch;
                }

                // we can protect edges to t2
                // because we already tested all cases where t2 is cut in the other branches
                const auto t2 = f->LabelToTerminal()[label2];
                if (not context->protectedEdges.contains(t2))
                {
                    edgeProtections.emplace(t2);
                    context->protectedEdges.emplace(t2);
                }
            }

            return RuleReturnCode::Continue;
        }
        case 2:
        {
            for (const auto& f : *instance)
            {
                const auto t2 = f->LabelToTerminal()[label2];
                if (t2->parent != nullptr)
                {
                    if (context->protectedEdges.contains(t2))
                    {
                        return RuleReturnCode::CutBranch;
                    }
                    changes.emplace(t2, f);
                    changes.top().doAction();
                }
            }
            return RuleReturnCode::Continue;
        }
        case 1:
        {
            auto pairSubtrees = std::unordered_map<std::shared_ptr<graph::Forest>, graph::Node*>();

            for (const auto& f : *instance)
            {
                for (auto pathDel : this->forestToPathDeletions[f])
                {
                    if (context->protectedEdges.contains(pathDel))
                    {
                        return RuleReturnCode::CutBranch;
                    }
                    changes.emplace(pathDel, f);
                    changes.top().doAction();

                }
                auto parent = f->LabelToTerminal()[label1]->parent;
                assert(parent == f->LabelToTerminal()[label2]->parent);
                pairSubtrees.emplace(f,parent);
            }

            auto nextRule = std::make_shared<solver::EqualPairReductionRule>(instance, context, pairSubtrees);
            nextRuleSuggestion = std::make_shared<std::list<std::shared_ptr<solver::AbstractRule>>>();
            nextRuleSuggestion->push_back(nextRule);

            return RuleReturnCode::ContinueWithRuleSuggestion;
        }
        default:
            throw std::logic_error("ABCBranchingRule : apply : undefined branch");
    }
}

void solver::ABCBranchingRule::unapply()
{
    if (not this->isApplied)
    {
        throw std::invalid_argument("ABCBranchingRule : unapply : rule is not applied");
    }
    isApplied = false;

    while (not changes.empty())
    {
        changes.top().undoAction();
        changes.pop();
    }

    if (branch == 3)
    {
        for (const auto& t : edgeProtections)
        {
            context->protectedEdges.erase(t);
        }
        edgeProtections.clear();
    }
}

std::shared_ptr<solver::AbstractRule>
solver::ABCBranchingRule::isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                            const std::shared_ptr<Context>& context)
{
    auto c = cuts_type();
    get<0>(c) = 0;
    get<1>(c) = 0;

    auto f = instance->at(0);
    for (const auto& [label, node] : f->LabelToTerminal())
    {
        if (node->sibling != nullptr and f->TerminalToLabel().contains(node->sibling))
        {
            get<0>(c) = label;
            get<1>(c) = node->sibling->smallestTerminal();
            get<2>(c).emplace(f, std::list<graph::Node*>());
            break;
        }
    }

    if (get<0>(c) == 0)
    {
        // we have a better rule for this case
        return nullptr;
    }

    // if there is a path in at least on instance, where something can be deleted
    bool existNonTrivialPath = false;

    for (unsigned int i = 1; i < instance->size(); i++)
    {
        auto fi = instance->at(i);
        auto t1 = fi->LabelToTerminal()[get<0>(c)];
        auto t2 = fi->LabelToTerminal()[get<1>(c)];
        auto root = fi->rootOf(t1);
        if (not t2->hasSubsetTerminals(root))
        {
            // we have a better rule for this case
            return nullptr;
        }

        // moving upwards from t1 to lca - and collect all cut-away subtrees
        auto cutAwaySubtrees = std::list<graph::Node*>();
        graph::Node* lca;
        graph::Node* it = t1;
        while (true)
        {
            assert(it->parent != nullptr);
            if (t2->hasSubsetTerminals(it->parent))
            {
                lca = it->parent;
                break;
            }
            cutAwaySubtrees.push_back(it->sibling);
            it = it->parent;
        }

        // moving upwards from t2 to lca - and collect all cut-away subtrees
        it = t2;
        while (true)
        {
            assert(it->parent != nullptr);
            if (it->parent == lca)
            {
                break;
            }
            cutAwaySubtrees.push_back(it->sibling);
            it = it->parent;
        }

        if (not cutAwaySubtrees.empty())
        {
            // actually the case == 1 result in another, better branching rule
            existNonTrivialPath = true;
        }
        get<2>(c).emplace(fi, std::move(cutAwaySubtrees));
    }

    if (not existNonTrivialPath)
    {
        return nullptr;
    }
    return std::make_shared<ABCBranchingRule>(instance, context, c);
}

std::shared_ptr<std::list<std::shared_ptr<solver::AbstractRule>>> solver::ABCBranchingRule::NextRuleSuggestion()
{
    return nextRuleSuggestion;
}

std::string solver::ABCBranchingRule::name() const
{
    return "ABCBranchingRule";
}

std::shared_ptr<solver::AbstractRule> solver::ABCBranchingRule::clone() const
{
    cuts_type c = {label1, label2, forestToPathDeletions};
    auto clone = std::make_shared<ABCBranchingRule>(instance, context, c);
    clone->branch = isApplied ? branch - 1 : branch;
    return clone;
}
