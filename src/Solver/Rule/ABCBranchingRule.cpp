#include "ABCBranchingRule.hpp"
#include "EqualPairReductionRule.hpp"

#include <cassert>

solver::ABCBranchingRule::ABCBranchingRule(
    const std::shared_ptr<graph::Instance>& instance,
    const std::shared_ptr<Context>& context,
    unsigned int aLabel,
    unsigned int cLabel,
    const std::list<std::pair<std::shared_ptr<graph::Forest>, std::list<graph::Node*>>>& forestWithBNodes) :
        AbstractBranchingRule(instance, context, 3),
        aLabel(aLabel),
        cLabel(cLabel),
        forestWithBNodes(forestWithBNodes)
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
            for (const auto& f: *instance)
            {
                const auto aNode = f->LabelToTerminal().at(aLabel);
                if (not aNode->parent)
                    continue;
                if (context->protectedEdges.contains(aNode))
                    return RuleReturnCode::CutBranch;
                changes.emplace(aNode, f);
                changes.top().doAction();


                // we can protect edges to cNode
                // because we already tested all cases where t2 is cut in the other branches
                const auto cNode = f->LabelToTerminal().at(cLabel);
                if (not context->protectedEdges.contains(cNode))
                {
                    edgeProtections.emplace(cNode);
                    context->protectedEdges.emplace(cNode);
                }

            }
            return RuleReturnCode::Continue;
        }
        case 2:
        {
            for (const auto& f : *instance)
            {
                const auto cNode = f->LabelToTerminal().at(cLabel);
                if (not cNode->parent)
                    continue;
                if (context->protectedEdges.contains(cNode))
                    return RuleReturnCode::CutBranch;
                changes.emplace(cNode, f);
                changes.top().doAction();
            }
            return RuleReturnCode::Continue;
        }
        case 1:
        {
            for (const auto& [f, bNodes] : forestWithBNodes)
            {
                for (auto bNode : bNodes)
                {
                    if (context->protectedEdges.contains(bNode))
                        return RuleReturnCode::CutBranch;
                    changes.emplace(bNode, f);
                    changes.top().doAction();
                }
                assert(f->LabelToTerminal()[aLabel]->parent == f->LabelToTerminal()[cLabel]->parent);
            }

            nextRuleSuggestion = std::make_shared<std::list<std::shared_ptr<solver::AbstractRule>>>();
            nextRuleSuggestion->push_back(std::make_shared<EqualPairReductionRule>(instance, context, aLabel, cLabel));

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
    unsigned int aLabel = 0;
    unsigned int cLabel = 0;
    auto forestWithBNodes = std::list<std::pair<std::shared_ptr<graph::Forest>, std::list<graph::Node*>>>();

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

    if (aLabel == 0)
    {
        // we have a better rule for this case
        // (CheckSingleVertexTreesRule, SingleVertexTreePropagationRule, EqualForestRule)
        return nullptr;
    }

    // if there is a path in at least on instance, where something can be deleted
    bool existNonTrivialPath = false;

    for (unsigned int i = 1; i < instance->size(); i++)
    {
        const auto& fi = instance->at(i);
        const auto& aNode = fi->LabelToTerminal().at(aLabel);
        const auto& cNode = fi->LabelToTerminal().at(cLabel);
        const auto& aRoot = fi->rootOf(aNode);
        if (not cNode->hasSubsetTerminals(aRoot))
        {
            // we have a better rule for this case (AC BranchingRule)
            return nullptr;
        }

        // moving upwards from aNode to lca - and collect b-nodes
        auto bNodes = std::list<graph::Node*>();
        graph::Node* lca;
        graph::Node* it = aNode;
        while (true)
        {
            assert(it->parent != nullptr);
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
            assert(it->parent != nullptr);
            if (it->parent == lca)
            {
                break;
            }
            bNodes.push_back(it->sibling);
            it = it->parent;
        }

        if (not bNodes.empty())
        {
            existNonTrivialPath = true;
            forestWithBNodes.emplace_back(fi, bNodes);
        }
    }

    if (not existNonTrivialPath)
    {
        // we have a better rule for this case (EqualPairRule)
        return nullptr;
    }
    return std::make_shared<ABCBranchingRule>(instance, context, aLabel, cLabel, forestWithBNodes);
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
    auto clone = std::make_shared<ABCBranchingRule>(instance, context, aLabel,cLabel, forestWithBNodes);
    clone->branch = isApplied ? branch - 1 : branch;
    return clone;
}
