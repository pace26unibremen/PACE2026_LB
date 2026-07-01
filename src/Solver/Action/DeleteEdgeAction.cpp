
#include "DeleteEdgeAction.hpp"

#include <algorithm>
#include <utility>

using namespace graph;
using namespace solver;

solver::DeleteEdgeAction::DeleteEdgeAction(graph::Node* child, const std::shared_ptr<graph::Forest>& forest) :
        forest(forest),
        child(child)
{
    if (not child->parent)
    {
        throw std::invalid_argument("DeleteEdgeAction : Constructor : 'child' has no parent node");
    }
}

void solver::DeleteEdgeAction::doAction()
{
    sibling = child->sibling;
    parent = child->parent;
    left = parent->leftChild;
    right = parent->rightChild;

    const auto leftRootIterator = std::ranges::find(forest->Roots(), parent);
    leftRoot_RootsIndex = std::distance(forest->Roots().begin(), leftRootIterator);
    parentIsRoot = (leftRootIterator != forest->Roots().end());

    if (parentIsRoot)
    {
        doParentIsRoot();
    }
    else
    {
        doParentIsInner();
    }

    #ifdef DEBUG_IMAGE_VIEW_GRAPH
    forest->renderImage();
    #endif
}

void DeleteEdgeAction::undoAction()
{
    if (parentIsRoot)
    {
        undoParentIsRoot();
    }
    else
    {
        undoParentIsInner();
    }

    #ifdef DEBUG_IMAGE_VIEW_GRAPH
    forest->renderImage();
    #endif
}

void DeleteEdgeAction::doParentIsRoot()
{
    // first new root is always at same position than the parent
    forest->Roots()[leftRoot_RootsIndex] = left;

    // position of second new root is somewhere after the old parent
    const auto rightRoot_Iterator =
        std::lower_bound(forest->Roots().begin() + leftRoot_RootsIndex, forest->Roots().end(), right,
                         [&](const Node* a, const Node* b) { return a->hasSmallestTerminal(b); });
    rightRoot_RootsIndex = std::distance(forest->Roots().begin(), rightRoot_Iterator);

    forest->Roots().insert(rightRoot_Iterator, right);

    sibling->sibling = nullptr;
    sibling->parent = nullptr;
    child->sibling = nullptr;
    child->parent = nullptr;
}

void DeleteEdgeAction::undoParentIsRoot()
{
    sibling->sibling = child;
    sibling->parent = parent;
    child->sibling = sibling;
    child->parent = parent;

    forest->Roots()[leftRoot_RootsIndex] = parent;
    forest->Roots().erase(forest->Roots().begin() + rightRoot_RootsIndex);
}

void DeleteEdgeAction::doParentIsInner()
{
    Node* grandParent = parent->parent;

    // Delete necessary Node by linking sibling to grandParent on the correct side
    if (grandParent->leftChild == parent)
    {
        grandParent->leftChild = sibling;
    }
    else
    {
        grandParent->rightChild = sibling;
    }

    // Reassign adjacent nodes to adjust the tree structure
    sibling->parent = grandParent;
    sibling->sibling = parent->sibling;
    parent->sibling->sibling = sibling;

    // Update subtreeTerminals up the tree, which now has child and it's subtrees removed
    const unsigned int subtreeTerminalsSize = child->subtreeTerminals.size();
    Node* traversedNode = parent;
    Node* root;

    // Traverse up to the root, updating subtreeTerminals and sorting children
    while (traversedNode != nullptr)
    {
        root = traversedNode;
        // Update subtreeTerminals by removing child's terminals
        for (unsigned int i = 0; i < subtreeTerminalsSize; i++)
        {
            // Use XOR to remove terminals
            traversedNode->subtreeTerminals[i] ^= child->subtreeTerminals[i];
        }

        // sort children
        Node& l = *traversedNode->leftChild;
        Node& r = *traversedNode->rightChild;

        if (r.hasSmallestTerminal(&l))
        {
            traversedNode->leftChild = &r;
            traversedNode->rightChild = &l;
        }

        traversedNode = traversedNode->parent;
    }

    // Insert new root nodes into forest's rootIndices

    // Get indices of the old root
    auto leftRoot_Iterator = std::ranges::find(forest->Roots(), root);
    leftRoot_RootsIndex = std::distance(forest->Roots().begin(), leftRoot_Iterator);
    Node* rootPtr = *leftRoot_Iterator;

    // If root has the smaller terminal, child goes after rootPtr, else before
    if (rootPtr->hasSmallestTerminal(child))
    {
        // Insert child after rootPtr
        auto rightRoot_Iterator =
            std::lower_bound(leftRoot_Iterator, forest->Roots().end(), child,
                             [&](const Node* a, const Node* b) { return a->hasSmallestTerminal(b); });
        rightRoot_RootsIndex = std::distance(forest->Roots().begin(), rightRoot_Iterator);
        forest->Roots().insert(rightRoot_Iterator, child);
    }
    else
    {
        // Insert child before rootPtr and therefor insert root after child
        *leftRoot_Iterator = child;
        auto rightRoot_Iterator =
            std::lower_bound(leftRoot_Iterator, forest->Roots().end(), root,
                             [&](const Node* a, const Node* b) { return a->hasSmallestTerminal(b); });
        rightRoot_RootsIndex = std::distance(forest->Roots().begin(), rightRoot_Iterator);
        forest->Roots().insert(rightRoot_Iterator, root);
    }

    // Detach the parent and sibling from the old child, which has become a root
    child->sibling = nullptr;
    child->parent = nullptr;
}

void DeleteEdgeAction::undoParentIsInner()
{
    Node* grandParent = parent->parent;

    // Reinsert the parent node between grandParent and sibling
    if (grandParent->leftChild == sibling)
    {
        grandParent->leftChild = parent;
    }
    else
    {
        grandParent->rightChild = parent;
    }

    // Reassign adjacent nodes to restore the original tree structure
    sibling->parent = parent;
    sibling->sibling = child;
    child->sibling = sibling;
    child->parent = parent;
    parent->sibling->sibling = parent;

    // Update subtreeTerminals up the tree, which now has child and it's subtrees added again
    const unsigned int subtreeTerminalsSize = child->subtreeTerminals.size();
    Node* traversedNode = parent;
    Node* root;

    // Traverse up to the root, updating subtreeTerminals and sorting children
    while (traversedNode != nullptr)
    {
        root = traversedNode;
        // Update subtreeTerminals by removing child's terminals
        for (unsigned int i = 0; i < subtreeTerminalsSize; i++)
        {
            // Use OR to add terminals back
            traversedNode->subtreeTerminals[i] |= child->subtreeTerminals[i];
        }

        // sort children
        Node& l = *traversedNode->leftChild;
        Node& r = *traversedNode->rightChild;

        if (r.hasSmallestTerminal(&l))
        {
            traversedNode->leftChild = &r;
            traversedNode->rightChild = &l;
        }

        traversedNode = traversedNode->parent;
    }

    // Override the prior added child with the restored root and erase the other root
    forest->Roots()[leftRoot_RootsIndex] = root;
    forest->Roots().erase(forest->Roots().begin() + rightRoot_RootsIndex);
}
