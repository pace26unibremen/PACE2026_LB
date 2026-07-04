#include "ChainReductionRule.hpp"
#include <ranges>

solver::ChainReductionRule::ChainReductionRule(const std::shared_ptr<graph::Instance>& instance,
                                               const std::shared_ptr<Context>& context,
                                               const std::list<std::pair<unsigned int, unsigned>>& reducedChains) :
        AbstractRule(instance, context, true),
        reducedChains(reducedChains)
{}

solver::RuleReturnCode solver::ChainReductionRule::apply()
{
    if (this->isApplied)
    {
        throw std::invalid_argument("ChainReductionRule : apply : rule is already applied");
    }
    isApplied = true;

    for (const auto& [lower, upper] : reducedChains)
    {
        for (const auto& forest : *instance)
        {
            changes.emplace(lower,upper,forest);
            changes.top().doAction();
        }

    }

    return RuleReturnCode::Continue;
}

void solver::ChainReductionRule::unapply()
{
    if (not this->isApplied)
    {
        throw std::invalid_argument("ChainReductionRule : unapply : rule is not applied");
    }
    isApplied = false;

    while (!changes.empty())
    {
        changes.top().undoAction();
        changes.pop();
    }
}

std::shared_ptr<solver::AbstractRule>
solver::ChainReductionRule::isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                         const std::shared_ptr<Context>& context)
{
    // S. Kelk et al. states that for multiple tree s
    // we can shorten chains to r := min{max{k, 3}, t + 1}
    // where t is the number of trees and k is solution size.
    // Since we don't know the solution size,
    // we use t+1 as upper bound on r.
    //
    // TODO we can also use our current upper bound on k

    const unsigned int lengthReducedChain = instance->size() + 1;

    // all labels that we already visisted
    std::unordered_set<unsigned int> visitedLabels;

    // maps labels, which are the starts point of a chains to their chains.
    // a chain is represented as a list of labels.
    std::unordered_map<unsigned int, std::list<unsigned int>> startToChain;

    // we iterate over labels, a try to build chains with the current label as starting point
    for (const auto& label : instance->at(0)->TerminalToLabel() | std::views::values)
    {
        if (visitedLabels.contains(label))
            continue;

        // insert label with a very trivial chain, that consist only of 'label'
        visitedLabels.insert(label);
        startToChain.insert({label, {label}});

        // For now, we exclude the case where a chain starts with a sibling pair and concentrate
        // on the case, where the chain starts in the middle of the forests.
        // But, if we would have a sibling pair with in at least one forest
        // then several cases are relevant:
        //
        // the sibling pair (l1,l2), with uncle l3
        //     ┌──┴──┐
        //   ┌─┴─┐   l3
        //  l1   l2
        //
        // case: another sibling pair  | case: chain with l1 /2       |  case: chain l1/l2 and uncle
        //                             |                              |        as sibling pair
        //       ┌──┴──┐               |     ┌──┴──┐                  |        ┌──┴──┐
        //     ┌─┴─┐   x               |   ┌─┴─┐   l3                 |    ┌───┴─┐   x
        //    l1   l2                  | ┌─┴┐  l1/l2                  |  l1/l2   l3
        // -> equal pair rule          |  -> chain (l1, l3, ...)      |  -> chain (l1, l3, ...)
        //
        //  case: chain starting with both
        //        ┌──┴──┐
        //     ┌──┴──┐   l3
        //   ┌─┴─┐   l1/l2
        // ┌─┴┐  l2/l1
        // -> chain (l1, l2, l3, ...)
        //
        // in some of these subvariants of these cases we should also consider B-, RB- and 2B-Rule

        // As said before, for now, we exclude the case where a chain starts with a sibling pair
        bool siblingStart = false;
        for (const auto& forest : *instance)
        {
            graph::Node* chainStart = forest->LabelToTerminal().at(label);
            if (forest->TerminalToLabel().contains(chainStart->sibling))
            {
                siblingStart = true;
            }
        }
        if (siblingStart)
            continue;

        // we try know to traverse the tree upwards and collect new chain elements (labels)
        unsigned int currentChainElement = label;
        while (true)
        {
            bool ongoingChain = true;
            unsigned int nextChainElement = 0;

            //     ┌───┴─────┐
            //   ┌─┴─┐      nextChainElement
            //    chainElement

            // we have to check the nextChainElement for each forests in our instance
            for (const auto& forest : *instance)
            {
                graph::Node* chainElementNode = forest->LabelToTerminal().at(currentChainElement);

                // for an ongoing chain we need an uncle
                // and the parent of the uncle shouldn't be a root node
                if (not chainElementNode->parent or                   // guard: we have parent
                    not chainElementNode->parent->sibling or          // guard: the parent has a sibling (uncle)
                    not chainElementNode->parent->parent->parent)     // guard: the grandparent is not a root
                {
                    ongoingChain = false;
                    break;
                }

                // the uncle must be a terminal
                auto uncle = chainElementNode->parent->sibling;
                if (not forest->TerminalToLabel().contains(uncle))
                {
                    ongoingChain = false;
                    break;
                }

                // the uncle must have the same label as the corresponding uncles in the other forests
                unsigned int uncleLabel = forest->TerminalToLabel().at(uncle);
                if (nextChainElement == 0)
                {
                    nextChainElement = uncleLabel;
                }
                else if (nextChainElement != uncleLabel)
                {
                    ongoingChain = false;
                    break;
                }
            }
            if (ongoingChain)
            {
                // ongoing chain: nextChainElement can be added to the current chain

                if (startToChain.contains(nextChainElement))
                {
                    // if we already have a chain that starts with nextChainElement, we merge both chains.
                    startToChain.at(label).splice(startToChain.at(label).end(), startToChain.at(nextChainElement));
                    startToChain.erase(nextChainElement);
                    // the current chain is finished after the merge (because the nextChainElement was finished)
                    break;
                }
                else
                {
                    // if nextChainElement does not start a new chain, we just add nextChainElement
                    // and continue building up this chain.
                    currentChainElement = nextChainElement;
                    startToChain.at(label).push_back(currentChainElement);
                    visitedLabels.insert(currentChainElement);
                }
            }
            else
            {
                // if chain is not ongoing we continue with the next unseen label
                // to check if this next label starts a chain.
                break;
            }
        }
    }

    std::list<std::pair<unsigned int, unsigned int>> reducedChainsStartEnd;

    for (const auto& [_, chain] : startToChain)
    {
        if (chain.size() <= lengthReducedChain)
            continue;

        // for the shortenChainAction we only need the last element that remains in the graph
        // and the last element of the chain.
        auto start = chain.begin();
        std::advance(start, lengthReducedChain - 1);
        reducedChainsStartEnd.emplace_back(*start, chain.back());
    }

    if (reducedChainsStartEnd.empty())
        return nullptr;
    return std::make_shared<ChainReductionRule>(instance, context, reducedChainsStartEnd);
}

std::string solver::ChainReductionRule::name() const
{
    return "ChainReductionRule";
}

std::shared_ptr<solver::AbstractRule> solver::ChainReductionRule::clone() const
{
    return std::make_shared<ChainReductionRule>(instance,context,reducedChains);
}
