#include "SubtreeReductionRule.hpp"

#include <algorithm>
#include <functional>
#include <queue>
#include <ranges>
#include <unordered_set>

#include <assert.h>

solver::SubtreeReductionRule::SubtreeReductionRule(const std::shared_ptr<graph::Instance>& instance,
                                                   const std::shared_ptr<Context>& context,
                                                   const std::unordered_map<std::shared_ptr<graph::Forest>,
                                                                            std::list<graph::Node*>>& forestToSubtrees) :
        AbstractRule(instance,context, true),
        forestToSubtrees(forestToSubtrees)
{}

solver::RuleReturnCode solver::SubtreeReductionRule::apply()
{
    if (this->isApplied)
    {
        throw std::invalid_argument("SubtreeReductionRule : apply : rule is already applied");
    }
    isApplied = true;

    for(const auto& [f,subtrees] : forestToSubtrees)
    {
        for (const auto& subtree : subtrees)
        {
            changes.emplace(subtree, f);
            changes.top().doAction();
        }
    }
    return RuleReturnCode::Continue;
}

void solver::SubtreeReductionRule::unapply()
{
    if (not this->isApplied)
    {
        throw std::invalid_argument("SubtreeReductionRule: unapply : rule is not applied");
    }
    isApplied = false;

    while (not changes.empty())
    {
        changes.top().undoAction();
        changes.pop();
    }
}

std::shared_ptr<solver::AbstractRule>
solver::SubtreeReductionRule::isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                           const std::shared_ptr<Context>& context)
{
    if (instance->size() <= 1) return nullptr;

    auto labels = instance->at(0)->LabelToTerminal();

    /// a set of all visited nodes over all forests, that where part of identical subtrees
    auto seenNodes_identical = std::unordered_set<graph::Node*>();
    /// a set of all visited nodes over all forests, that weren't part of identical subtrees
    auto seenNodes_diff = std::unordered_set<graph::Node*>();

    // this function adds recursively all ancestors of the nodes to the seenNodes_diff set
    std::function<void(const std::vector<graph::Node*>)> addAncestorSeenDiffNodes =
        [&seenNodes_diff, &addAncestorSeenDiffNodes]
        (const std::vector<graph::Node*>& nodes) {
        std::vector<graph::Node*> parents;
        parents.reserve(nodes.size());
        for (const auto& node : nodes)
        {
            const auto& parent = node->parent;
            if (parent != nullptr and not seenNodes_diff.contains(parent))
            {
                seenNodes_diff.emplace(parent);
                parents.push_back(parent);
            }
        }
        if (not parents.empty()) addAncestorSeenDiffNodes(parents);
    };

    // A queue of already found identical subtrees.
    // An element of the stack is a vector of the subtree roots for each forest.
    // s.t. within a vector all nodes come from different forests and are roots of identical subtrees.
    auto identicalSubtreeRoots = std::queue<std::vector<graph::Node*>>();

    // A list that saves all maximum subtree roots
    auto maximumSubtree = std::list<std::vector<graph::Node*>>();

    // first we fill the 'identicalSubtreeRoot' queue with the trivial terminal subtrees
    for (const auto& label : labels | std::views::keys)
    {
        // all nodes for the current label
        auto terminalsForLabel = std::vector<graph::Node*>();
        terminalsForLabel.reserve(instance->size());

        for (const auto& f : *instance)
        {
            assert(f->LabelToTerminal().contains(label));
            auto terminal = f->LabelToTerminal()[label];
            if (seenNodes_identical.contains(terminal))
            {
                // this might happen if the forests already have collapsed subtrees
                // we also assume that all forests of the instance were subject to the same collapsing actions
                break;
            }
            seenNodes_identical.emplace(terminal);
            terminalsForLabel.push_back(terminal);
        }
        if (not terminalsForLabel.empty()) identicalSubtreeRoots.push(terminalsForLabel);
    }

    // now we traverse simultaneously on all forests from the terminals upwards
    // until we reached a maximal subtree roots
    while (not identicalSubtreeRoots.empty())
    {
        auto currentSubtree = identicalSubtreeRoots.front();
        identicalSubtreeRoots.pop();

        if (std::any_of(currentSubtree.begin(), currentSubtree.end(),
            [](const graph::Node* n){return n->parent == nullptr;}))
        {
            // This case:
            // We have an actual root as root of a subtree
            // so the current subtree is maximal
            maximumSubtree.push_back(currentSubtree);
            addAncestorSeenDiffNodes(currentSubtree);
        }
        else if (std::all_of(currentSubtree.begin(), currentSubtree.end(),
            [&seenNodes_identical](const graph::Node* n){return seenNodes_identical.contains(n->sibling);}))
        {
            // This case:
            // Each node in current is not actual root, so we have siblings.
            // Each sibling is root of some subtree that also occurs in each other forest
            // and is therefore an identical subtrees, and it follows that the siblings are 'seen'.
            //
            // But we have no guarantee that the subtrees under the siblings are the same,
            // we have to check:
            auto compareSibling = currentSubtree.at(0)->sibling;
            if (std::all_of(currentSubtree.begin(), currentSubtree.end(),
            [&compareSibling](const graph::Node* n){return compareSibling->hasSameTerminals(n->sibling);}))
            {
                // If all siblings have the same subtree terminals (and therefore identical subtrees)
                // then their parents are also identical subtrees.
                if (not seenNodes_identical.contains(currentSubtree[0]->parent))
                {
                    auto parent = currentSubtree | std::views::transform([](const graph::Node* n)
                        { return n->parent; });
                    identicalSubtreeRoots.push({parent.begin(), parent.end()});
                    seenNodes_identical.insert(parent.begin(), parent.end());
                }
            }
            else
            {
                // But if the siblings are roots of different subtrees,
                // then current subtree is maximal.
                maximumSubtree.push_back(currentSubtree);
                addAncestorSeenDiffNodes(currentSubtree);
            }
        }
        else if (std::none_of(currentSubtree.begin(), currentSubtree.end(),
        [&seenNodes_identical](const graph::Node* n){return seenNodes_identical.contains(n->sibling);}))
        {
            // This case:
            // Each node in current is not actual root, so we have siblings.
            if (std::none_of(currentSubtree.begin(), currentSubtree.end(),
                             [&seenNodes_diff](const graph::Node* n)
                             { return seenNodes_diff.contains(n->sibling); }))
            {
                // But no sibling was seen before, so we don't know if current is maximal or not.
                // We have to handle this case later.
                identicalSubtreeRoots.emplace(currentSubtree);
            }
            else
            {
                // There are siblings which are marked as 'not a part of an identical subtree'
                // So current is maximal
                maximumSubtree.push_back(currentSubtree);
                addAncestorSeenDiffNodes(currentSubtree);
            }
        }
        else
        {
            // This case:
            // Each node in current is not actual root, so we have siblings.
            // Some Siblings have been seen before, and some not.
            // Which means they can't represent the same subtree.
            // So current subtree is maximal.
            maximumSubtree.push_back(currentSubtree);
            addAncestorSeenDiffNodes(currentSubtree);
        }
    }

    if (not maximumSubtree.empty())
    {
        auto forestToSubtrees =std::unordered_map<std::shared_ptr<graph::Forest>, std::list<graph::Node*>>();
        // transform the list of maximumSubtree to a mapping where each forest gets its own subtree roots
        for (unsigned int i = 0; i < instance->size(); i++)
        {
            const auto& forest = instance->at(i);
            auto allSubtreesOfForest = std::list<graph::Node*>();

            for (const auto& identicalSubtrees : maximumSubtree)
            {
                // filter trivial subtrees
                if (forest->TerminalToLabel().contains(identicalSubtrees[i])) continue;

                allSubtreesOfForest.push_back(identicalSubtrees[i]);
            }
            if (allSubtreesOfForest.empty()) continue;
            forestToSubtrees.emplace(forest, allSubtreesOfForest);
        }
        if (forestToSubtrees.empty()) return nullptr;
        return std::make_shared<SubtreeReductionRule>(instance, context, forestToSubtrees);
    }

    return nullptr;
}

std::string solver::SubtreeReductionRule::name() const
{
    return "SubtreeReductionRule";
}
