#include "TwinRelation.hpp"

#include "LeastCommonAncestor.hpp"

#include <ranges>

namespace cluster
{

TwinRelation::TwinRelation(const std::shared_ptr<graph::Instance>& instance)
{

    LCAs = std::vector<std::shared_ptr<cluster::LeastCommonAncestor>>();

    // We generate an LCA for each forest and for convenience
    for (auto& forest : *instance)
    {
        auto LCA = std::make_shared<cluster::LeastCommonAncestor>(cluster::LeastCommonAncestor((forest)));
        LCAs.push_back(LCA);
    }

    // This likely escalates runtime from linear to square. However, this is likely to be unavoidable.

    for (unsigned int homeForestIndex = 0; homeForestIndex < instance->size(); ++homeForestIndex)
    {

        for (unsigned int foreignForestIndex = 0; foreignForestIndex < LCAs.size(); ++foreignForestIndex)
        {
            if (homeForestIndex == foreignForestIndex)
                continue;
            // Syncs leafs in the Twin-Buffer first.
            prepareLeafTwins(instance->at(homeForestIndex), instance->at(foreignForestIndex));
            //Syncs interior twins up to the root.
            generateInteriorTwinRelation(instance->at(homeForestIndex)->Roots().front(), LCAs.at(foreignForestIndex));
            // Flushes Twin-Buffer and merges it to the big table.
            fuseTwinBufferToSets();
        }
    }
}

// Man, I don't know. We don't achieve as many cluster points as rSPR, though rSPR is quite scuffed in that regard.
void TwinRelation::generateInteriorTwinRelation(graph::Node* givenNode,
                                                const std::shared_ptr<cluster::LeastCommonAncestor>& foreignLCA)
{

    if (givenNode->leftChild == nullptr && givenNode->rightChild == nullptr)
    {
        if (not nodeToTwinBuffer.contains(givenNode))
            throw std::logic_error("The leaf twins are not properly initialized.");
        return;
    }

    // Apparently what happens from now on is a mystical magical left-fold.
    if (const auto leftChild = givenNode->leftChild)
    {
        generateInteriorTwinRelation(leftChild, foreignLCA);
        nodeToTwinBuffer[givenNode] = nodeToTwinBuffer[leftChild];
    }

    if (const auto rightChild = givenNode->rightChild)
    {
        generateInteriorTwinRelation(rightChild, foreignLCA);

        graph::Node* twin =
            foreignLCA->getLeastCommonAncestor(nodeToTwinBuffer[givenNode], nodeToTwinBuffer[rightChild]);
        nodeToTwinBuffer[givenNode] = twin;
    }
}

// Takes the elements of the TwinBuffer (1:1) and fuses them to the "big table" (1:[InstanceSize])
void TwinRelation::fuseTwinBufferToSets()
{
    for (const auto& pair : nodeToTwinBuffer) nodeToTwins[pair.first].insert(pair.second);

    nodeToTwinBuffer.clear();
}

// This functions existence is being justified because its name provides what actually happens here.
void TwinRelation::prepareLeafTwins(const std::shared_ptr<graph::Forest>& homeForest,
                                    const std::shared_ptr<graph::Forest>& foreignForest)
{

    for (const auto& label : homeForest->LabelToTerminal() | std::views::keys)
        nodeToTwinBuffer[homeForest->LabelToTerminal()[label]] = foreignForest->LabelToTerminal()[label];
}

}  //namespace cluster
