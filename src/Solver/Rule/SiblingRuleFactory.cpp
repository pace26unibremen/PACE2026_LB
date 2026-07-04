#include "SiblingRuleFactory.hpp"

#include "ABCBranchingRule.hpp"
#include "ACBranchingRule.hpp"
#include "BRule.hpp"
#include "EqualPairReductionRule.hpp"
#include "ForcedCherryCutRule.hpp"
#include "GreedyCherryCutRule.hpp"
#include "ReverseBRule.hpp"
#include "TwoBRule.hpp"

#include <cassert>

std::pair<unsigned int, unsigned int> solver::SiblingRuleFactory::getSiblings(
    const std::shared_ptr<graph::Instance>& instance,
    const std::shared_ptr<solver::Context>& context)
{
    const auto& f = instance->at(0);
    for (const auto& label : context->heuristicLabelOrder)
    {
        if (not f->LabelToTerminal().contains(label))
        {
            continue;
        }
        const auto& node = f->LabelToTerminal().at(label);

        if (node->sibling != nullptr and f->TerminalToLabel().contains(node->sibling))
        {
            return {f->TerminalToLabel().at(node), f->TerminalToLabel().at(node->sibling)};
        }
    }
    return {0,0};
}

std::shared_ptr<solver::AbstractRule>
solver::SiblingRuleFactory::basicRules(const std::shared_ptr<graph::Instance>& instance,
                                     const std::shared_ptr<solver::Context>& context)
{
    const auto& [aLabel, cLabel] = getSiblings(instance, context);
    auto forestWithBNodes = std::list<std::pair<std::shared_ptr<graph::Forest>, std::list<graph::Node*>>>();

    if (aLabel == 0 or cLabel == 0)
    {
        // there is no sibling pair in f0
        return nullptr;
    }

    // if the siblings from f0 are siblings in every other forest.
    bool siblings = true;

    for (unsigned int i = 1; i < instance->size(); i++)
    {
        const auto& fi = instance->at(i);
        const auto& aNode = fi->LabelToTerminal().at(aLabel);
        const auto& cNode = fi->LabelToTerminal().at(cLabel);

        // If the nodes are siblings, we can continue, as neither the AC, nor bNodes for the ABC rule can be found
        if (aNode->sibling == cNode)
        {
            continue;
        }
        siblings = false;

        const auto& aRoot = fi->rootOf(aNode);
        if (not cNode->hasSubsetTerminals(aRoot))
        {
            return std::make_shared<ACBranchingRule>(instance, context, aLabel, cLabel);
        }

        // moving upwards from aNode to lca - and collect b-nodes
        auto bNodes = std::list<graph::Node*>();
        graph::Node* lca;
        graph::Node* it = aNode;
        while (true)
        {
            if (cNode->hasSubsetTerminals(it->parent))
            {
                lca = it->parent;
                break;
            }
            bNodes.push_back(it->sibling);
            it = it->parent;
        }

        // moving upwards from cNode to lca - and collect b-nodes
        it = cNode;
        while (true)
        {
            if (it->parent == lca)
            {
                break;
            }
            bNodes.push_back(it->sibling);
            it = it->parent;
        }
        forestWithBNodes.emplace_back(fi, bNodes);
    }

    if (siblings)
    {
        return std::make_shared<EqualPairReductionRule>(instance, context, aLabel, cLabel);
    }

    return std::make_shared<ABCBranchingRule>(instance, context, aLabel, cLabel, forestWithBNodes);
}


std::pair<graph::Node*, graph::Node*>
solver::SiblingRuleFactory::checkBNodes(graph::Node* const & aNode, graph::Node* const & cNode)
{
    if (aNode->parent and aNode->parent->sibling == cNode)
    {
        //   ┌──┴──┐
        // ┌─┴─┐   c
        // a   b
        return {aNode->sibling, nullptr};
    }
    if (cNode->parent and cNode->parent->sibling == aNode)
    {
        //   ┌──┴──┐
        // ┌─┴─┐   a
        // c   b
        return {cNode->sibling, nullptr};
    }
    if (aNode->parent and aNode->parent->parent and aNode->parent->parent->sibling == cNode)
    {
        //      ┌───┴──┐
        //   ┌──┴─┐    c
        // ┌─┴─┐  b2
        // a  b1
        return {aNode->sibling, aNode->parent->sibling};
    }
    if (cNode->parent and cNode->parent->parent and cNode->parent->parent->sibling == aNode)
    {
        //      ┌───┴──┐
        //   ┌──┴─┐    a
        // ┌─┴─┐  b2
        // c  b1
        return {cNode->sibling, cNode->parent->sibling};
    }
    return {nullptr, nullptr};
}

std::shared_ptr<solver::AbstractRule> solver::SiblingRuleFactory::allRules(const std::shared_ptr<graph::Instance>& instance,
                                                                             const std::shared_ptr<solver::Context>& context)
{
    const auto& [aLabel, c1Label] = getSiblings(instance, context);
    auto forestWithBNodes = std::list<std::pair<std::shared_ptr<graph::Forest>, std::list<graph::Node*>>>();

    if (aLabel == 0 and c1Label == 0)
    {
        // there is no sibling pair in f0
        return nullptr;
    }

    // c2 is the potential uncle of the sibling pair in f0
    // which corresponds to the c-node for the reversed B rule after Whidden
    // and for the 2B rule it would correspond to the x-node after Whidden.
    // However, throughout this method, we will continue to use the symbol 'c2'.
    const unsigned int c2Label = [&instance, &aLabel]() -> unsigned int
    {
        const auto& f0 = instance->at(0);
        const auto& aNode = f0->LabelToTerminal().at(aLabel);
        if (f0->TerminalToLabel().contains(aNode->parent->sibling))
        {
            return f0->TerminalToLabel().at(aNode->parent->sibling);
        }
        return 0;
    }();

    // label for one of the potential b nodes for the B rule or the 2B Rule
    // It remains zero until we reach the point where we have to decide on the value.
    unsigned int b1Label = 0;
    // label for one of the potential b nodes for the B rule or the 2B Rule
    // It remains zero until we reach the point where we have to decide on the value.
    unsigned int b2Label = 0;

    // if the siblings from f0 are siblings in every other forest.
    bool equalPairRule = true;

    // if the checked subtree always have c2 as uncle
    // and that the a-node and c1+c2 are separated by the same pendant subtrees b1 / b2
    bool twoBRule = (c2Label != 0);

    // if a-node and c2-node are a sibling pair in at least one forest
    // and the other forests have the same subtree ((a c1) c2) as f0.
    bool reverseBRule = (c2Label != 0);

    // label for the potential nodes that will be cutted by the reversed B rule.
    // This label is either aLabel or c1Label. We will decide later which one.
    unsigned int reverseBLabel = 0;

    // if a-node and c1-node are separated by one pendant subtree and that this subtree is b1 or b2
    bool bRule = true;

    // We check now how the a-node and c1-node are distributed in the other forests
    for (unsigned int i = 1; i < instance->size(); i++)
    {
        const auto& fi = instance->at(i);
        const auto& aNode = fi->LabelToTerminal().at(aLabel);
        const auto& c1Node = fi->LabelToTerminal().at(c1Label);

        // check if we have the same sibling pair in fi as in f0
        if (aNode->sibling == c1Node)
        {
            // if 2B rule or reverse B rule are still in the race
            // we need to check that the sibling pair's uncle is c2
            //   ┌──┴──┐
            // ┌─┴─┐   c2 ?
            // a   c1
            if (twoBRule or reverseBRule)
            {
                if ((not fi->TerminalToLabel().contains(aNode->parent->sibling)) or
                    (fi->TerminalToLabel().at(aNode->parent->sibling) != c2Label))
                {
                    twoBRule = false;
                    reverseBRule = false;
                }
            }
            // we can move to next forest
            continue;
        }
        // From here on, we have the case that a-node and c1-node are no sibling pairs in fi.
        equalPairRule = false;

        // If a-node and c1-node are in different components in at least one forest,
        // then AC Branching Rule is the only option.
        const auto& aRoot = fi->rootOf(aNode);
        if (not c1Node->hasSubsetTerminals(aRoot))
        {
            return std::make_shared<ACBranchingRule>(instance, context, aLabel, c1Label);
        }

        // reverse B rule: we need to check if a-node and c2-node are a sibling pair
        //     f0             f i      /    fi
        //   ┌──┴──┐                  /              siblingNode == a  and reverseBNode == c1 (left)
        // ┌─┴─┐   c2       ┌─┴─┐    /    ┌─┴─┐      siblingNode == c1 and reverseBNode == a  (right)
        // a   c1           a   c2  /    c1   c2
        if (reverseBRule)
        {
            const auto& c2Node = fi->LabelToTerminal().at(c2Label);
            if (not c2Node->sibling)
                reverseBRule = false;
            const auto& siblingNode = c2Node->sibling;

            // 3 cass: siblingNode == aNode, siblingNode == c1Node, siblingNode == other
            if (siblingNode == aNode)
            {
                // if we have not decided, if aNode or c1Node is the reversed b
                // we can do this now.
                // otherwise we must ensure, that we decided for the c1-label.
                if (reverseBLabel == 0)
                    reverseBLabel = c1Label;
                else if (reverseBLabel != c1Label)
                    reverseBRule = false;
            }
            else if (siblingNode == c1Node)
            {
                // if we have not decided, if aNode or c1Node is the reversed b
                // we can do this now.
                // otherwise we must ensure, that we decided for a-label.
                if (reverseBLabel == 0)
                    reverseBLabel = aLabel;
                else if (reverseBLabel != aLabel)
                    reverseBRule = false;
            }
            else
            {
                // we cannot apply reverse B rule
                // if the sibling of c2 is not a-node or c1 node
                reverseBRule = false;
            }
        }

        // 2B rule: we need to check that c2 is an uncle of a-node or c1-node.
        if (twoBRule)
        {
            const auto& c2Node = fi->LabelToTerminal().at(c2Label);
            // (not c2 'is uncle of' a) and
            // (not c2 'is uncle of' c1)
            if ((not aNode->parent or aNode->parent->sibling != c2Node) and
                (not c1Node->parent or c1Node->parent->sibling != c2Node))
            {
                twoBRule = false;
            }
        }

        // B rule and 2B rule: we need to check the b nodes.
        // What follows is a long block where b1Label and b2Label are assigned
        // and/or the labels for of the potential b-nodes in the current forest are verified.
        const auto& [b1Node, b2Node] = checkBNodes(aNode, c1Node);
        if (not b1Node)
        {
            // current case:
            // a and c1 are connected, but there are more than two subtrees 'between' a and c1
            bRule = false;
            twoBRule = false;
        }
        else if (not b2Node)
        {
            // current case:
            //   ┌──┴──┐     /   ┌──┴──┐
            // ┌─┴─┐   c1   /  ┌─┴─┐   a
            // a   b1Node  /  c1   b1Node
            // at this point, we haven't checked anything about the label of the b1 Node

            // we can fill the b-nodes list for the ACB Branching rule
            forestWithBNodes.push_back({fi,{b1Node}});

            // from here on, we check B rule and 2B rule stuff, so if none of them is possible anymore -> next forest
            if (not twoBRule and not bRule)
            {
                continue;
            }

            if (not fi->TerminalToLabel().contains(b1Node))
            {
                // b1Node is not a terminal
                bRule = false;
                twoBRule = false;
                continue;
            }

            const auto possibleBLabel = fi->TerminalToLabel().at(b1Node);
            if (possibleBLabel == b1Label or possibleBLabel == b2Label)
            {
                // if the possibleBLabel matches the stored labels for b1 or for b2 everything is fine
                continue;
            }
            if (b1Label == 0)
            {
                // if we have not decided on the label of b1 until now,
                // we can assign the label of the current b node
                b1Label = possibleBLabel;
                continue;
            }
            if (b2Label == 0)
            {
                // if we have not decided on the label of b2 until now,
                // we can assign the label of the current b node
                b2Label = possibleBLabel;
                continue;
            }
            // if we have already decided on the values of b1 and b2 and
            // if the current b label does not match the stored values for b1 and b2
            // then we can not apply B and 2B rule.
            bRule = false;
            twoBRule = false;
            continue;
        }
        else
        {
            // current case:
            //       ┌───┴──┐     /          ┌───┴──┐
            //    ┌──┴─┐    c1   /        ┌──┴─┐    a
            //  ┌─┴─┐  b2       /       ┌─┴─┐  b2
            //  a  b1          /       c1   b1
            // at this point, we haven't checked anything about the label of the b1 Node

            // we fill the b-nodes list for the ACB branching rule
            forestWithBNodes.push_back({fi,{b1Node, b2Node}});

            // B rule can not be applied anymore
            bRule = false;

            // from here on, we only check 2B rule stuff, so if 2  is not possible, we can skip this
            if (not twoBRule)
            {
                continue;
            }

            if (not fi->TerminalToLabel().contains(b1Node) or
                not fi->TerminalToLabel().contains(b2Node))
            {
                // b1Node or b2Node is not a terminal
                twoBRule = false;
                continue;
            }
            const auto possibleB1Label = fi->TerminalToLabel().at(b1Node);
            const auto possibleB2Label = fi->TerminalToLabel().at(b2Node);

            // check possibleB1Label first:
            if (possibleB1Label != b1Label and possibleB1Label != b2Label)
            {
                if (b1Label == 0)
                {
                    // if b1 was not assigned until now, we can do this now
                    b1Label = possibleB1Label;
                }
                else if (b2Label == 0)
                {
                    // if b2 was not assigned until now, we can do this now
                    b2Label = possibleB1Label;
                }
                else
                {
                    // possibleB1Label didn't match previous assignments for b1Label/b2Label.
                    twoBRule = false;
                    continue;
                }
            }

            // check now possibleB2Label:
            if (possibleB2Label == b1Label or possibleB2Label == b2Label)
            {
                continue;
            }
            if (b1Label == 0)
            {
                // if b1 was not assigned until now, we can do this now
                b1Label = possibleB2Label;
                continue;
            }
            if (b2Label == 0)
            {
                // if b2 was not assigned until now, we can do this now
                b2Label = possibleB2Label;
                continue;
            }
            // possibleB1Label didn't match previous assignments for b1/b2.
            twoBRule = false;
            continue;
        }
        // Here ends the block for the B / 2B rule. :)

        // The current situation in fi is:
        // a and c1 are connected, but there are more than two subtrees 'between' a and c1.
        // (see first condition of previous B/2B block)
        // This is the ABC branching rule case, so we collect all b-nodes between a and c1.

        // moving upwards from aNode to lca - and collect b-nodes
        auto bNodes = std::list<graph::Node*>();
        graph::Node* lca;
        graph::Node* it = aNode;
        while (true)
        {
            assert(it->parent != nullptr);
            if (c1Node->hasSubsetTerminals(it->parent))
            {
                lca = it->parent;
                break;
            }
            bNodes.push_back(it->sibling);
            it = it->parent;
        }

        // moving upwards from cNode to lca - and collect b-nodes
        it = c1Node;
        while (true)
        {
            assert(it->parent != nullptr);
            if (it->parent == lca)
            {
                break;
            }
            bNodes.push_back(it->sibling);
            it = it->parent;
        }

        forestWithBNodes.emplace_back(fi, bNodes);
    }

    if (equalPairRule)
    {
        return std::make_shared<EqualPairReductionRule>(instance, context, aLabel, c1Label);
    }
    if (bRule)
    {
        return std::make_shared<BRule>(instance, context, b1Label, b2Label);
    }
    if (reverseBRule)
    {
        return std::make_shared<ReverseBRule>(instance, context, reverseBLabel);
    }
    if (twoBRule)
    {
        // there is an edge case, that we didn't checked until now:
        // b1 or b2 could be c2.
        // This is because when we checked that c2 is an uncle of the subtree
        // we actually only checked if it's an uncle of the a-node or an uncle of the c1-node
        // regardless which one is the higher node.
        // Essentially, twoBRule could be true in this situation:
        //       ┌─────┴────┐
        //    ┌──┴────┐     c1
        //  ┌─┴─┐   b2==c2        -> c2 is the uncle of a, but we need c2 to be an uncle of c1
        //  a   b1
        // But 2B is not applicable, c2 should be the uncle of c1 for B2 rule

        if (b1Label != c2Label and b2Label != c2Label)
        {
            return std::make_shared<TwoBRule>(instance, context, std::pair(b1Label, b2Label));
        }
    }

    return std::make_shared<ABCBranchingRule>(instance, context, aLabel, c1Label, forestWithBNodes);
}

std::shared_ptr<solver::AbstractRule>
solver::SiblingRuleFactory::approximationRule(const std::shared_ptr<graph::Instance>& instance,
                                              const std::shared_ptr<solver::Context>& context)
{
    const auto& [aLabel, cLabel] = getSiblings(instance, context);
    if (aLabel == 0 or cLabel == 0)
    {
        // No sibling pair in forest 0 — the single-vertex rules handle this.
        return nullptr;
    }

    // Check that first cherry against all other forests once.
    for (unsigned int i = 1; i < instance->size(); i++)
    {
        const auto& fi = instance->at(i);
        if (fi->LabelToTerminal().at(aLabel)->sibling != fi->LabelToTerminal().at(cLabel))
        {
            // (a, c) conflicts: cut it. Two trees get the sharp Whidden-Zeh 3-approximation cut,
            // t > 2 gets the general greedy cut.
            if (instance->size() == 2)
            {
                return std::make_shared<ForcedCherryCutRule>(instance, context, aLabel, cLabel);
            }
            return std::make_shared<GreedyCherryCutRule>(instance, context, aLabel, cLabel);
        }
    }

    // (a, c) is a cherry in every forest: contract it.
    return std::make_shared<EqualPairReductionRule>(instance, context, aLabel, cLabel);
}
