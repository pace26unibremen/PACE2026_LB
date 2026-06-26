#include "DecoupleSubtreeAction.hpp"

#include "DeleteEdgeAction.hpp"

#include <algorithm>

solver::DecoupleSubtreeAction::DecoupleSubtreeAction(const std::shared_ptr<graph::Forest>& forest,
                                                     graph::Node* decouplingPoint, unsigned int newLabelParentTree,
                                                     unsigned int newLabelSubtree) :
        forest(forest),
        decouplingPoint(decouplingPoint),
        newLabelParentTree(newLabelParentTree),
        newLabelSubtree(newLabelSubtree)
{}

void solver::DecoupleSubtreeAction::xorLabelPropagation(const std::vector<uint64_t>& labels, graph::Node* start)
{
    // Update subtreeTerminals up the tree, which now has child and it's subtrees removed
    const unsigned int subtreeTerminalsSize = start->subtreeTerminals.size();
    graph::Node* traversedNode = start;

    // Traverse up to the root, updating subtreeTerminals and sorting children
    while (traversedNode != nullptr)
    {
        // Update subtreeTerminals by removing child's terminals
        for (unsigned int i = 0; i < subtreeTerminalsSize; i++)
        {
            // Use XOR to remove terminals
            traversedNode->subtreeTerminals[i] ^= labels[i];
        }

        // sort children
        graph::Node* l = traversedNode->leftChild;
        graph::Node* r = traversedNode->rightChild;

        if (l and r and r->hasSmallestTerminal(l))
        {
            traversedNode->leftChild = r;
            traversedNode->rightChild = l;
        }
        traversedNode = traversedNode->parent;
    }
}

void solver::DecoupleSubtreeAction::doAction()
{
    // build the root part of the decoupled subtree
    decoupledSubtreeRoot = *decouplingPoint;
    decoupledSubtreeRoot.parent = &decoupledSubtreeVirtualRoot;
    decoupledSubtreeRoot.sibling = &decoupledSubtreeVirtualSibling;
    if (decoupledSubtreeRoot.leftChild) decoupledSubtreeRoot.leftChild->parent = &decoupledSubtreeRoot;
    if (decoupledSubtreeRoot.rightChild) decoupledSubtreeRoot.rightChild->parent = &decoupledSubtreeRoot;

    decoupledSubtreeVirtualSibling.parent = &decoupledSubtreeVirtualRoot;
    decoupledSubtreeVirtualSibling.sibling = &decoupledSubtreeRoot;
    decoupledSubtreeVirtualSibling.subtreeTerminals = std::vector<uint64_t>(decoupledSubtreeRoot.subtreeTerminals.size());
    decoupledSubtreeVirtualSibling.subtreeTerminals[(newLabelSubtree-1) / 64] = (uint64_t) 1 << (newLabelSubtree - 1) % 64;

    decoupledSubtreeVirtualRoot.leftChild = &decoupledSubtreeRoot;
    decoupledSubtreeVirtualRoot.rightChild = &decoupledSubtreeVirtualSibling;
    decoupledSubtreeVirtualRoot.subtreeTerminals = decoupledSubtreeRoot.subtreeTerminals;
    decoupledSubtreeVirtualRoot.subtreeTerminals[(newLabelSubtree-1) / 64] ^= (uint64_t) 1 << (newLabelSubtree - 1) % 64;

    // remove connection from parent tree to subtree
    decouplingPoint->leftChild = nullptr;
    decouplingPoint->rightChild = nullptr;

    // update subtreeTerminals vector
    auto bitmask = decoupledSubtreeRoot.subtreeTerminals;
    bitmask[(newLabelParentTree-1) / 64] ^= (uint64_t) 1 << (newLabelParentTree - 1) % 64;
    xorLabelPropagation(bitmask, decouplingPoint);

    // update label-terminal maps
    forest->LabelToTerminal()[newLabelParentTree] = decouplingPoint;
    forest->TerminalToLabel()[decouplingPoint] = newLabelParentTree;
    forest->LabelToTerminal()[newLabelSubtree] = &decoupledSubtreeVirtualSibling;
    forest->TerminalToLabel()[&decoupledSubtreeVirtualSibling] = newLabelSubtree;

    // manage the root vector: insert new root at correct position, if necessary rearrange root of parent tree

    // find root node of parent tree
    auto parentTreeRootIt = std::find_if(forest->Roots().begin(), forest->Roots().end(),
                                      [&](const graph::Node* r) { return decouplingPoint->hasSubsetTerminals(r); });
    graph::Node* parentTreeRoot = *parentTreeRootIt;

    if (parentTreeRoot->hasSmallestTerminal(&decoupledSubtreeVirtualRoot))
    {
        // root vector before [..., parentTreeRoot, ...,                                 ]
        //             after  [..., parentTreeRoot, ..., decoupledSubtreeVirtualRoot, ...]
        auto decoupledSubtreeIt =
            std::lower_bound(parentTreeRootIt, forest->Roots().end(), &decoupledSubtreeVirtualRoot,
                             [&](const graph::Node* a, const graph::Node* b) { return a->hasSmallestTerminal(b); });
        forest->Roots().insert(decoupledSubtreeIt, &decoupledSubtreeVirtualRoot);
    }
    else
    {
        // root vector before [..., parentTreeRoot             , ...,                   ]                 ]
        //             after  [..., decoupledSubtreeVirtualRoot, ..., parentTreeRoot ...]
        *parentTreeRootIt = &decoupledSubtreeVirtualRoot;
        auto newParentTreeRootIt =
            std::lower_bound(parentTreeRootIt, forest->Roots().end(), parentTreeRoot,
                             [&](const graph::Node* a, const graph::Node* b) { return a->hasSmallestTerminal(b); });
        forest->Roots().insert(newParentTreeRootIt, parentTreeRoot);
    }
}

void solver::DecoupleSubtreeAction::undoAction()
{
    // For the undo-process it is important that the decoupled subtree and the parent tree may have changed.
    // We want to couple the part of the subtree that has the decoupledSubtreeVirtualSibling
    // (the artificial terminal with the label newLabelSubtree) as a sibling to its root.
    // If decoupledSubtreeVirtualSibling has no sibling, we don't couple parent tree and subtree, but we cut the
    // decoupling point from the parent tree and forget it together with decoupledSubtreeVirtualSibling.
    // Else we want to merge the decoupling point with the root denoted by the decoupledSubtreeVirtualSibling.

    // the decoupling point may have changed (especially due to another decoupled subtree)
    decouplingPoint = forest->LabelToTerminal()[newLabelParentTree];

    if(forest->LabelToTerminal()[newLabelSubtree] != &decoupledSubtreeVirtualSibling)
    {
        throw std::runtime_error("newLabelSubtree has not decoupledSubtreeVirtualSibling as terminal");
    }
    if(forest->TerminalToLabel()[&decoupledSubtreeVirtualSibling] != newLabelSubtree)
    {
        throw std::runtime_error("decoupledSubtreeVirtualSibling has not newLabelSubtree as label");
    }

    if (decoupledSubtreeVirtualSibling.sibling == nullptr)
    {
        undoWithoutSubtreeRoot();
    }
    else
    {
        undoWithSubtreeRoot();
    }
}

void solver::DecoupleSubtreeAction::undoWithSubtreeRoot()
{
    // the node we want to 'merge' with the decouplingPoint
    auto subtreePartRoot = decoupledSubtreeVirtualSibling.sibling;

    if(decoupledSubtreeVirtualSibling.sibling->sibling != &decoupledSubtreeVirtualSibling)
    {
        throw std::runtime_error("decoupledSubtreeVirtualSibling.sibling.sibling is not decoupledSubtreeVirtualSibling");
    }
    if (decoupledSubtreeVirtualSibling.parent != &decoupledSubtreeVirtualRoot or
        subtreePartRoot->parent != &decoupledSubtreeVirtualRoot or
        decoupledSubtreeVirtualRoot.leftChild != subtreePartRoot or
        decoupledSubtreeVirtualRoot.rightChild != &decoupledSubtreeVirtualSibling)
    {
        throw std::runtime_error("decoupledSubtreeVirtualRoot is not correct root of subtreePartRoot / decoupledSubtreeVirtualSibling");
    }

    // manage root vector: remove decoupledSubtreeVirtualRoot and  if necessary rearrange root of parent tree
    auto parentTreeRootIt = std::ranges::find_if(forest->Roots(),
                                      [&](const graph::Node* r) { return r->hasTerminal(newLabelParentTree); });
    auto subtreeRootIt = std::ranges::find(forest->Roots(), &decoupledSubtreeVirtualRoot);

    if (subtreeRootIt == forest->Roots().end())
    {
        throw std::runtime_error("decoupledSubtreeVirtualRoot is not correct root of subtreePartRoot / decoupledSubtreeVirtualSibling");
    }

    graph::Node* parentTreeRoot = *parentTreeRootIt;
    if (parentTreeRoot->hasSmallestTerminal(subtreePartRoot))
    {
        // root vector before [..., parentTreeRoot, ..., subtreePartRoot, ... ]
        //             after  [..., parentTreeRoot, ...                       ]
        forest->Roots().erase(subtreeRootIt);
    }
    else
    {
        // root vector before [..., subtreePartRoot, ..., parentTreeRoot, ... ]
        //             after  [..., parentTreeRoot,  ...                      ]
        forest->Roots().erase(parentTreeRootIt);
        *subtreeRootIt = parentTreeRoot;
    }

    // label-terminals maps
    forest->LabelToTerminal().erase(newLabelSubtree);
    forest->LabelToTerminal().erase(newLabelParentTree);
    forest->TerminalToLabel().erase(&decoupledSubtreeVirtualSibling);
    forest->TerminalToLabel().erase(decouplingPoint);

    // if the subtree part is a single vertex tree, then decoupling point becomes a terminal again
    if (forest->TerminalToLabel().contains(subtreePartRoot))
    {
        unsigned int label = forest->TerminalToLabel()[subtreePartRoot];
        forest->TerminalToLabel().erase(subtreePartRoot);

        forest->TerminalToLabel()[decouplingPoint] = label;
        forest->LabelToTerminal()[label] = decouplingPoint;
    }

    // subtreeTerminals
    xorLabelPropagation(subtreePartRoot->subtreeTerminals, decouplingPoint);

    // connections
    decouplingPoint->leftChild = subtreePartRoot->leftChild;
    decouplingPoint->rightChild = subtreePartRoot->rightChild;
    if (decouplingPoint->leftChild)
    {
        decouplingPoint->leftChild->parent = decouplingPoint;
    }
    if (decouplingPoint->rightChild)
    {
        decouplingPoint->rightChild->parent = decouplingPoint;
    }
}


void solver::DecoupleSubtreeAction::undoWithoutSubtreeRoot()
{
    // decoupledSubtreeVirtualSibling has no sibling, so it is a single vertex tree (svt).
    // Since decoupledSubtreeVirtualSibling annotated as sibling the preserved root, we actually don't have
    // a preserved root, which means that we cannot connect the decoupled subtree with the parent tree.
    // Instead, we cut the decouplingPoint from the parent tree.
    // Then we just forget the two svt decoupledSubtreeVirtualSibling and decouplingPoint.

    // We need to ensure, that after `undo` no new nodes:
    //   - decoupledSubtreeVirtualSibling
    //   - decoupledSubtreeVirtualRoot
    //   - decoupledSubtreeRoot
    // are reachable parts in the tree (because they will be garbage collected when this rules ends living).
    //   - decoupledSubtreeVirtualSibling is a svt, we will remove it from the root list, etc.
    //   - decoupledSubtreeVirtualRoot should never be reachable, because decoupledSubtreeVirtualSibling was made a svt
    //   - decoupledSubtreeRoot could a valid root, the node gets the memory location of decouplingPoint

    auto decoupledSubtreeVirtualRootIt = std::find(forest->Roots().begin(), forest->Roots().end(), &decoupledSubtreeVirtualRoot);
    if (decoupledSubtreeVirtualRootIt != forest->Roots().end())
    {
        throw std::runtime_error("undoWithoutSubtreeRoot : decoupledSubtreeVirtualRoot is reachable part of the forest");
    }

    // make decouplingPoint a single vertex tree
    if (decouplingPoint->parent != nullptr)
    {
        DeleteEdgeAction(decouplingPoint,forest).doAction();
    }

     // manage roots vector
    auto decoupledSubtreeVirtualSiblingIt = std::find(forest->Roots().begin(), forest->Roots().end(), &decoupledSubtreeVirtualSibling);
    if (decoupledSubtreeVirtualSiblingIt != forest->Roots().end())
    {
        forest->Roots().erase(decoupledSubtreeVirtualSiblingIt);
    }
    else
    {
        throw std::runtime_error("undoWithoutSubtreeRoot : decoupledSubtreeVirtualSiblingIt is not a root");
    }

    auto decouplingPointIt = std::find(forest->Roots().begin(), forest->Roots().end(), decouplingPoint);
    if (decouplingPointIt != forest->Roots().end())
    {
        forest->Roots().erase(decouplingPointIt);
    }
    else
    {
        throw std::runtime_error("undoWithoutSubtreeRoot : decouplingPointIt is not a root");
    }

    // label-terminal maps
    forest->LabelToTerminal().erase(newLabelSubtree);
    forest->LabelToTerminal().erase(newLabelParentTree);
    forest->TerminalToLabel().erase(&decoupledSubtreeVirtualSibling);
    forest->TerminalToLabel().erase(decouplingPoint);


    // swap decouplingPoint and decoupledSubtreeRoot
    // so that decoupledSubtreeRoot gets the valid memory in the nodes vector
    auto decoupledSubtreeRootIt = std::find(forest->Roots().begin(), forest->Roots().end(), &decoupledSubtreeRoot);
    std::swap(*decouplingPoint, decoupledSubtreeRoot);
    if (decoupledSubtreeRootIt != forest->Roots().end())
    {
        *decoupledSubtreeRootIt = decouplingPoint;
        if (decouplingPoint->leftChild != nullptr)
        {
            decouplingPoint->leftChild->parent = decouplingPoint;
        }
        if (decouplingPoint->rightChild != nullptr)
        {
            decouplingPoint->rightChild->parent = decouplingPoint;
        }
        if (forest->TerminalToLabel().contains(&decoupledSubtreeRoot))
        {
            forest->TerminalToLabel()[decouplingPoint] = forest->TerminalToLabel()[&decoupledSubtreeRoot];
            forest->LabelToTerminal()[forest->TerminalToLabel()[&decoupledSubtreeRoot]] = decouplingPoint;
            forest->TerminalToLabel().erase(&decoupledSubtreeRoot);
        }
    }

}