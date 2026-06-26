#include "ClusterPointGenerator.hpp"

namespace cluster
{

// This does not generate the same amount of Cluster Points as rSPR, but they should be very much valid.
// The engaged code reviewer may take a look at rSPR and tell me what's wrong (or right?) :)
void ClusterPointGenerator::generateClusterPoints(graph::Node* node)
{
    // Recurse
    if (node->leftChild)
        generateClusterPoints(node->leftChild);
    if (node->rightChild)
        generateClusterPoints(node->rightChild);
    // Root / Non-Forking / Child Check
    if (node->parent == nullptr)
        return;
    if (node->leftChild == nullptr or node->rightChild == nullptr)
        return;
    // We're very strict about potential cluster points having the same depth within the trees.
    // Actually, doing this may save a lot of comparisons later.
    // int nodeHeight = checkHeightOfNode(node);
    // for (const auto& twin : twinRelation.nodeToTwins[node])
    //     if (nodeHeight != checkHeightOfNode(twin)) return;
    // Heavy Invariant Checks (Same Leaf Set for class / True Twin Class)
    if (not leafEquivalent(node))
        return;  // We may want to remove this later!
    if (not trueEquivalenceClass(node))
        return;

    // No dealbreakers => Cluster Point.
    clusterPoints.push_back(node);
}

ClusterPointGenerator::ClusterPointGenerator(const std::shared_ptr<graph::Instance>& instance,
                                             const cluster::TwinRelation& twinRelation) :
        twinRelation(twinRelation)
{
    generateClusterPoints(instance->front()->Roots().front());
}

bool ClusterPointGenerator::trueEquivalenceClass(graph::Node* node) const
{
    // Generate the reference set: The cluster point node and its twins.
    const auto& setOfTwins = twinRelation.nodeToTwins.at(node);

    std::set<graph::Node*> referenceSet(setOfTwins);
    referenceSet.insert(node);

    // The equivalence holds if every twin-of-twin is already in referenceSet.
    for (const auto& twin : setOfTwins)
        for (const auto& twinOfTwin : twinRelation.nodeToTwins.at(twin))
            if (!referenceSet.contains(twinOfTwin))
                return false;  // early exit

    return true;
}

bool ClusterPointGenerator::leafEquivalent(graph::Node* node) const
{
    const auto& classOfTwins = twinRelation.nodeToTwins.at(node);

    for (const auto& twin : classOfTwins)
    {
        if (!node->hasSameTerminals(twin))
            return false;

        for (const auto& twinOfTwin : twinRelation.nodeToTwins.at(twin))
            if (!node->hasSameTerminals(twinOfTwin))
                return false;
    }

    return true;
}

ClusterPointGenerator ClusterPointGenerator::wrappedConstructor(const std::shared_ptr<graph::Instance>& instance)
{
    auto table = cluster::TwinRelation(instance);
    return {ClusterPointGenerator(instance, table)};
}

}  //namespace cluster
