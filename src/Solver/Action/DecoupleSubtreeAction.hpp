#ifndef PACE2026_DECOUPLE_SUBTREE_ACTION_HPP
#define PACE2026_DECOUPLE_SUBTREE_ACTION_HPP

#include "../../Graph/Forest.hpp"
#include "AbstractAction.hpp"

namespace solver
{

/// \brief This Action decouples a subtree from a tree.
/// The subtree is defined by its root (named \ref decouplingPoint), which is an inner node of the tree.
/// The \ref decouplingPoint becomes a new terminal in the parent tree with the label \ref newLabelParentTree.
/// The action constructs a new node to represent the root of the decoupled subtree (named \ref decoupledSubtreeRoot).
/// To preserve and identify the correct root in the undo-process, \ref decoupledSubtreeRoot gets an artificial terminal
/// sibling;
/// therefore, two additional new nodes are constructed: \ref decoupledSubtreeVirtualRoot will be placed on
/// top of \ref decoupledSubtreeRoot and \ref decoupledSubtreeVirtualSibling will be the artificial sibling and
/// terminal with label \ref newLabelSubtree.
///
/// \dotfile DecoupleSubtreeAction.dot "Concept"
class DecoupleSubtreeAction : public solver::AbstractAction
{
  private:
    /// \brief The forest on which the action is performed.
    std::shared_ptr<graph::Forest> forest;

    /// \brief This node identifies the point where the tree is split into parent tree and subtree.
    /// Before action this node represents the root of the subtree.
    /// After action this node is a terminal in the parent tree with label \ref newLabelParentTree.
    graph::Node* decouplingPoint;

    /// \brief The root node of the decoupled subtree.
    graph::Node decoupledSubtreeRoot = graph::Node();

    /// \brief This node is the sibling of \ref decoupledSubtreeRoot and a terminal with label \ref newLabelSubtree.
    /// \ref decoupledSubtreeVirtualRoot and \ref decoupledSubtreeVirtualSibling are placed on top of the subtree.
    graph::Node decoupledSubtreeVirtualSibling = graph::Node();

    /// \brief This node is the parent of \ref decoupledSubtreeRoot and \ref decoupledSubtreeVirtualSibling.
    /// It is placed on top of the decoupled subtree together with \ref  decoupledSubtreeVirtualSibling.
    graph::Node decoupledSubtreeVirtualRoot = graph::Node();

    /// \brief The label of \ref decoupledSubtreeVirtualSibling
    unsigned int newLabelParentTree;

    /// \brief A new label for the newly created terminal \ref decoupledSubtreeVirtualSibling
    unsigned int newLabelSubtree;

    /// \brief Performs a bitwise xor with \ref newLabels on \ref graph::Node::subtreeTerminals on
    /// \ref start and on all its ancestors and reorders the children accordingly.
    /// \param labels xor bitmask
    /// \param start starting point for upward propagation
    static void xorLabelPropagation(const std::vector<uint64_t>& labels, graph::Node* start);

    /// \brief undo action in case that the decoupled subtree has \b no preserved root
    void undoWithoutSubtreeRoot();

    /// \brief undo action in case that the decoupled subtree has root
    void undoWithSubtreeRoot();

  public:
    /// \brief Constructs a DecoupleSubtreeAction on \ref forest.
    /// \param forest The forest on which the action is performed.
    /// \param decouplingPoint identifies the point where the tree is split into parent tree and subtree.
    /// \param newLabelParentTree the new label of the remaining stump in the parent tree.
    /// \param newLabelSubtree the artificial new label that becomes a sibling of the decoupled subtree root.
    DecoupleSubtreeAction(
        const std::shared_ptr<graph::Forest>& forest,
        graph::Node* decouplingPoint,
        unsigned int newLabelParentTree,
        unsigned int newLabelSubtree);

    void doAction() override;

    void undoAction() override;
};

}  //namespace solver

#endif  //PACE2026_DECOUPLE_SUBTREE_ACTION_HPP
