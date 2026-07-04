//
// Created by kaufm on 29.06.2026.
//

#ifndef PACE2026_SHORTENCHAINACTION_HPP
#define PACE2026_SHORTENCHAINACTION_HPP
#include "AbstractAction.hpp"
#include "../../Graph/Forest.hpp"

namespace solver
{

    /// \brief Shortens a chain of terminals from \ref lower to \ref upper.
    /// The shorted chain contains only lower.
    /// All other chain elements are completely reduced from the forest.
    ///
    /// Undoing the action restores the chain, such that the removed elements are placed on top of lower again.
    ///
    /// \code            p
    ///              ┌───┴──┐
    ///          ┌───┴──┐   y                p
    ///       ...     upper       =>      ┌──┴─┐
    ///   ┌──┴─┐                        ┌─┴─┐  y
    /// ┌─┴─┐  lower + 1                x   lower
    /// x   lower
    class ShortenChainAction : public AbstractAction
    {
    private:
        /// \brief label of the lower terminal in the chain
        unsigned int lower;
        /// \brief label of the upper terminal in the chain
        unsigned int upper;
        /// \brief the forest
        std::shared_ptr<graph::Forest> forest;

        /// \brief The node, that is the parent of the upper-node (before calling \ref doAction)
        graph::Node* upperParent;
        /// \brief The node, that is the uncle of the lower-node (before calling \ref doAction)
        graph::Node* lowerUncle;

        /// \brief map of all reduced labels with their nodes from \ref forest::labelToTerminal
        std::unordered_map<unsigned int, graph::Node*> reducedLabelToTerminal;

        /// \brief map of all reduced terminals with their labels from \ref forest::terminalToLabel
        std::unordered_map<graph::Node*, unsigned int> reducedTerminalToLabel;

        /// \brief bitmask that encodes all reduced labels
        std::vector<uint64_t> bitmaskReduceSubtreeTerminals;

        /// \brief helper that applies \ref bitmaskReduceSubtreeTerminals with XOR on all ancestors of \ref node
        /// and resort the children of each ancestor.
        /// Traverses from node to root.
        void propagateBitmaskToRoot(graph::Node* node) const;

        /// \brief recalculates the subtreeTerminals and the children order of the reduced chain elements
        /// Traverses from upperNode to lowerNode.
        unsigned int recalculateChainSubtreeTerminals(graph::Node* upperNode, graph::Node* lowerNode);

    public:
        ShortenChainAction(unsigned int lower, unsigned int upper, const std::shared_ptr<graph::Forest>& forest);

        void doAction() override;

        void undoAction() override;

    };

}

#endif  //PACE2026_SHORTENCHAINACTION_HPP
