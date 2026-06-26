#include "CollapseSubtreeAction.hpp"

solver::CollapseSubtreeAction::CollapseSubtreeAction(graph::Node* node, const std::shared_ptr<graph::Forest>& forest) :
    forest(forest),
    node(node)
{}

void solver::CollapseSubtreeAction::doAction()
{
    leftChild  = node->leftChild;
    rightChild = node->rightChild;

    node->leftChild = nullptr;
    node->rightChild = nullptr;

    unsigned int smallestLabel = 0;
    int k = 0;
    for(uint64_t bitmask : node->subtreeTerminals)
    {
        while (bitmask != 0) {
            unsigned i = __builtin_ctzll(bitmask);
            bitmask &= bitmask - 1;

            unsigned int label = 64 * k + i + 1;
            if(smallestLabel == 0)
            {
                smallestLabel = label;
            }

            graph::Node* oldNode = forest->LabelToTerminal()[label];
            collapsedLabelToTerminals.emplace(label, oldNode);
            forest->LabelToTerminal()[label] = node;
            if (forest->TerminalToLabel().contains(oldNode) and forest->TerminalToLabel()[oldNode] == label)
            {
                collapsedTerminals.emplace(oldNode, label);
            }
            forest->TerminalToLabel().erase(oldNode);
        }
        k++;
    }
    collapsedLabel = smallestLabel;
    forest->TerminalToLabel().emplace(node, collapsedLabel);

    #ifdef DEBUG_IMAGE_VIEW_GRAPH
    forest->renderImage();
    #endif
}

void solver::CollapseSubtreeAction::undoAction()
{
    // the node may have changed (e.g., due to cluster reduction)
    node = forest->LabelToTerminal()[collapsedLabel];

    node->leftChild = leftChild;
    node->rightChild = rightChild;
    leftChild->parent = node;
    rightChild->parent = node;

    for(const auto& [label, oldNode] : collapsedLabelToTerminals)
    {
        forest->LabelToTerminal()[label] = oldNode;
    }
    for ( const auto& [oldNode, label] : collapsedTerminals)
    {
        forest->TerminalToLabel()[oldNode] = label;
    }

    forest->TerminalToLabel().erase(node);

    #ifdef DEBUG_IMAGE_VIEW_GRAPH
    forest->renderImage();
    #endif
}
