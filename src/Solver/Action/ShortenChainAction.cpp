//
// Created by kaufm on 29.06.2026.
//

#include "ShortenChainAction.hpp"
#include <algorithm>
#include <ranges>
#include <vector>

namespace solver
{

     ShortenChainAction::ShortenChainAction(unsigned int lower, unsigned int upper,
         const std::shared_ptr<graph::Forest>& forest) :lower(lower), upper(upper), forest(forest)
    {}

    void ShortenChainAction::propagateBitmaskToRoot(graph::Node* node) const
    {
         // Traverse up to the root, updating subtreeTerminals and sorting children
         while (node != nullptr)
         {
             // Update subtreeTerminals by removing child's terminals
             for (unsigned int i = 0; i < bitmaskReduceSubtreeTerminals.size(); i++)
             {
                 // Use OR to add terminals back
                 node->subtreeTerminals[i] ^= bitmaskReduceSubtreeTerminals[i];
             }

             // sort children
             graph::Node& l = *node->leftChild;
             graph::Node& r = *node->rightChild;

             if (r.hasSmallestTerminal(&l))
             {
                 node->leftChild = &r;
                 node->rightChild = &l;
             }

             node = node->parent;
         }
     }

    unsigned int ShortenChainAction::recalculateChainSubtreeTerminals(graph::Node* upperNode, graph::Node* lowerNode)
     {
         // recursive function

         // end of recursion: if we reach a terminal or lowerNode
         if (!upperNode->leftChild || upperNode == lowerNode)
         {
             return upperNode->smallestTerminal();
         }

         // recursion calls
         unsigned int leftMinLabel = recalculateChainSubtreeTerminals(upperNode->leftChild, lowerNode);
         unsigned int rightMinLabel = recalculateChainSubtreeTerminals(upperNode->rightChild, lowerNode);

         // order siblings
         if (leftMinLabel > rightMinLabel)
         {
             std::swap(upperNode->leftChild, upperNode->rightChild);
         }

         // recalculate subtreeTerminals as bitwise or of both children
         for(unsigned int i = 0; i < upperNode->subtreeTerminals.size(); i++)
         {
             upperNode->subtreeTerminals[i] =
                 upperNode->leftChild->subtreeTerminals[i] |
                 upperNode->rightChild->subtreeTerminals[i];
         }

         // propagate the smallest label
         return std::min(leftMinLabel, rightMinLabel);
     }

    void ShortenChainAction::doAction()
    {
         auto upperNode = forest->LabelToTerminal().at(upper);
         auto lowerNode = forest->LabelToTerminal().at(lower);

         auto chainParent = upperNode->parent->parent;
         auto chainSibling = upperNode->parent->sibling;

         auto lowerParent = lowerNode->parent;

         upperParent = upperNode->parent;    // we store these nodes to get the reduced chain elements back
         lowerUncle = lowerParent->sibling;  // in the undoAction method

         //                        chainParent
         //                       ┌───┴─────┐
         //                   ┌───┴─────┐   chainSibling
         //               ┌───┴─────┐   upper (last reduced chain element)
         //           ┌───┴─────┐
         //       ┌───┴─────┐
         //  lowerParent   lowerUncle (first reduced chain element)
         //   ┌───┴─┐
         //        lower

         // We want to cut at lowerParent and upperParent glue chainParent and lowerParent together

         lowerParent->sibling = chainSibling;
         lowerParent->parent = chainParent;

         chainSibling->sibling = lowerParent;

         if (chainParent->leftChild == chainSibling) chainParent->rightChild = lowerParent;
         else chainParent->leftChild = lowerParent;

         // take care of the subtreeTerminals
         bitmaskReduceSubtreeTerminals = lowerParent->subtreeTerminals;
         for (unsigned int i = 0; i < bitmaskReduceSubtreeTerminals.size(); i++)
         {
             bitmaskReduceSubtreeTerminals[i] ^= upperNode->parent->subtreeTerminals[i];
         }
         propagateBitmaskToRoot(chainParent);

         // take care of the ordering in the roots vector
         auto root = forest->rootOf(chainParent);
         auto root_Iterator = std::ranges::find(forest->Roots(), root);
         forest->Roots().erase(root_Iterator, root_Iterator+1);

         root_Iterator = std::lower_bound(forest->Roots().begin(), forest->Roots().end(), root,
                             [&](const graph::Node* a, const graph::Node* b) { return a->hasSmallestTerminal(b); });
         forest->Roots().insert(root_Iterator, root);

         // get all labels that we removed
         std::unordered_set<unsigned int> chainLabels;
         for (size_t block = 0; block < bitmaskReduceSubtreeTerminals.size(); ++block) {
             uint64_t value = bitmaskReduceSubtreeTerminals[block];

             while (value != 0) {
                 uint64_t t = value & -value;
                 int bit_index = __builtin_ctzll(value);
                 unsigned int number = block * 64 + bit_index + 1;
                 chainLabels.insert(number);
                 value ^= t;
             }
         }

         // store all entries from LabelToTerminal and TerminalToLabel the we will remove
         for (auto label : chainLabels)
         {
             reducedLabelToTerminal.emplace(label, forest->LabelToTerminal().at(label));
             auto n = forest->LabelToTerminal().at(label);
             reducedTerminalToLabel.emplace(n, forest->TerminalToLabel().at(n));
         }

         // remove the reduced labels / terminals from the maps
         std::erase_if(forest->TerminalToLabel(), [chainLabels](const auto& entry){return chainLabels.contains(entry.second);});
         std::erase_if(forest->LabelToTerminal(), [chainLabels](const auto& entry){return chainLabels.contains(entry.first);});
    }

    void ShortenChainAction::undoAction()
    {
         // case 'lower has a parent and a grandparent'
         //        chain parent
         //      ┌───┴─────┐
         // lower parent   chain Sibling
         //  ┌───┴─┐
         //      lower
         //
         // We want to insert the chain above lower parent and below chain parent.
         //
         // The highest chain element (upperParent) will be connected with chainParent (child-parent)
         // and with chainSibling (sibling-sibling).
         //
         // The lowest REDUCED chain element (lowerUncle / 'lower+1') will be connected with lowerParent
         // (sibling-sibling), accordingly lowerParent will be connected to the parent of lowerUncle (child-parent).

         auto lowerNode = forest->LabelToTerminal().at(lower);
         graph::Node* root;

         if (lowerNode->parent && lowerNode->parent->parent)
         {
             auto lowerParent = lowerNode->parent;
             auto chainParent = lowerParent->parent;
             auto chainSibling = lowerParent->sibling;


             lowerUncle->sibling = lowerParent;
             lowerParent->sibling = lowerUncle;

             lowerParent->parent = lowerUncle->parent;

             if (lowerUncle->parent->leftChild == lowerUncle) lowerUncle->parent->rightChild = lowerParent;
             else lowerUncle->parent->leftChild = lowerParent;


             upperParent->parent = chainParent;
             upperParent->sibling = chainSibling;

             chainSibling->sibling = upperParent;
             if (chainParent->leftChild == chainSibling) chainParent->rightChild = upperParent;
             else chainParent->leftChild = upperParent;

             // We remove the root here, because it may does not satisfy the order of the roots vector.
             // We will reinsert the root later at the correct position.
             root = forest->rootOf(lowerNode);
             auto rootIterator = std::ranges::find(forest->Roots(), root);
             forest->Roots().erase(rootIterator,rootIterator+1);
         }
         // case 'lower has a parent and this parent is a root'
         //
         // lower parent
         //  ┌─┴─┐
         //     lower
         //
         // We want to insert the chain above lower parent.
         //
         // The highest chain element (upper->parent) becomes a root.

         // The lowest REDUCED chain element (lowerUncle / 'lower+1') will be connected with lowerParent
         // (sibling-sibling), accordingly lowerParent will be connected to the parent of lowerUncle (child-parent).

         else if (lowerNode->parent)
         {
             auto& lowerParent = lowerNode->parent;

             lowerUncle->sibling = lowerParent;
             lowerParent->sibling = lowerUncle;

             lowerParent->parent = lowerUncle->parent;
             if (lowerUncle->parent->leftChild == lowerUncle) lowerUncle->parent->rightChild = lowerParent;
             else lowerUncle->parent->leftChild = lowerParent;

             upperParent->parent = nullptr;
             upperParent->sibling = nullptr;

             // We remove the root here, because it may does not satisfy the order of the roots vector.
             // We will reinsert the root later at the correct position.
             root = upperParent;
             auto rootIterator = std::ranges::find(forest->Roots(), lowerParent);
             forest->Roots().erase(rootIterator,rootIterator+1);
         }
         else
         {
             // case 'lower is a single vertex tree'
             //
             // We want to insert the chain above lower.
             //
             // The highest chain element (upper->parent) becomes a root.
             //
             // The lowest REDUCED chain element (lowerUncle / 'lower+1') will be connected with lower
             // (sibling-sibling), accordingly lower will be connected to the parent of lowerUncle (child-parent).

             lowerNode->sibling = lowerUncle;
             lowerNode->parent = lowerUncle->parent;
             lowerUncle->sibling = lowerNode;
             if (lowerUncle->parent->leftChild == lowerUncle) lowerUncle->parent->rightChild = lowerNode;
             else lowerUncle->parent->leftChild = lowerNode;

             upperParent->parent = nullptr;
             upperParent->sibling = nullptr;

             // We remove the root here, because it may does not satisfy the order of the roots vector.
             // We will reinsert the root later at the correct position.
             root = upperParent;
             auto rootIterator = std::ranges::find(forest->Roots(), lowerNode);
             forest->Roots().erase(rootIterator,rootIterator+1);
         }

         // take care of the subtree terminals and the ordering of children
         recalculateChainSubtreeTerminals(upperParent,lowerUncle->sibling);
         propagateBitmaskToRoot(upperParent->parent);

         // reinsert the root
         auto rootIterator = std::lower_bound(forest->Roots().begin(), forest->Roots().end(), root,
                             [&](const graph::Node* a, const graph::Node* b) { return a->hasSmallestTerminal(b); });
         forest->Roots().insert(rootIterator, root);

         // reinsert the removed entries to LabelToTerminal and TerminalToLabel
         for (const auto& [label,node] : reducedLabelToTerminal)
             forest->LabelToTerminal().insert({label,node});
         for (const auto& [node, label] : reducedTerminalToLabel)
             forest->TerminalToLabel().insert({node,label});
    }


}  //namespace solver
