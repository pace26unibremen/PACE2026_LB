#ifndef PACE2026_Collapse_SUBTREE_ACTION_HPP
#define PACE2026_Collapse_SUBTREE_ACTION_HPP

#include "AbstractAction.hpp"
#include "../../Graph/Forest.hpp"
#include "../../Graph/Node.hpp"

namespace solver
{

/// \brief collapses a subtree into a single new leaf node
class CollapseSubtreeAction : AbstractAction
{
  private:
    /// \brief forest on which the action will be performed
    std::shared_ptr<graph::Forest> forest;

    /// \brief A Pointer to the node from which the subtree will be collapsed into a single leaf
    graph::Node* node;

    /// \brief The label that remains at the collapsed subtree leaf (i.e., the smallest label of the subtree)
    unsigned int collapsedLabel;

    /// \brief A Pointer to the left child of node
    graph::Node* leftChild;

    /// \brief A Pointer to the right child of node
    graph::Node* rightChild;

    /// \brief store collapsed label to terminals
    std::unordered_map<unsigned int, graph::Node*> collapsedLabelToTerminals = std::unordered_map<unsigned int, graph::Node*>();

    /// \brief store collapsed label to terminals
    std::unordered_map<graph::Node*, unsigned> collapsedTerminals = std::unordered_map<graph::Node*, unsigned int>();

  public:
    /// \param node The node from which the subtree will be collapsed into single leaf.
    /// \param forest A shared pointer to the forest on which the action will be performed.
    CollapseSubtreeAction(graph::Node* node, const std::shared_ptr<graph::Forest>& forest);

    void doAction() override;

    void undoAction() override;

};

}  //namespace solver

#endif  //PACE2026_Collapse_SUBTREE_ACTION_HPP
