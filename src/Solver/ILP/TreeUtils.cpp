#include "TreeUtils.hpp"

#include <cassert>
#include <stdexcept>
#include <algorithm>

namespace solver {

int getRootIndex(std::shared_ptr<graph::Forest> forest)
{
    int rootIndex = -1;
    int rootCount = 0;

    for(int i = 0; i < (int)forest->Nodes().size(); ++i)
    {
        if(forest->Nodes()[i].parent == nullptr)
        {
            rootIndex = i;
            rootCount++;
        }
    }

    // Exactly one root node must exist
    assert(rootCount == 1);

    if(rootIndex == -1)
        throw std::logic_error("TreeUtils::getRootIndex: no root node found");

    return rootIndex;
}

int getLCA(unsigned int leaf1, unsigned int leaf2,
           const graph::Forest& forest,
           cluster::LeastCommonAncestor& lca,
           const std::unordered_map<const graph::Node*, int>& nodeToIndex)
{
    // Labels should exist...
    assert(forest.LabelToTerminal().count(leaf1) > 0);
    assert(forest.LabelToTerminal().count(leaf2) > 0);

    graph::Node* node1 = forest.LabelToTerminal().at(leaf1);
    graph::Node* node2 = forest.LabelToTerminal().at(leaf2);

    // Compute LCA of Nodes...
    graph::Node* lcaNode = lca.getLeastCommonAncestor(node1, node2);

    // Return Index...
    assert(nodeToIndex.count(lcaNode) > 0);
    return nodeToIndex.at(lcaNode);
}

std::unordered_map<const graph::Node*, int> buildNodeToIndexMap(const graph::Forest& forest)
{
    std::unordered_map<const graph::Node*, int> nodeToIndex;

    for(int i = 0; i < (int)forest.Nodes().size(); ++i)
        nodeToIndex[&forest.Nodes()[i]] = i;

    // Each Node should have a unique index...
    assert(nodeToIndex.size() == forest.Nodes().size());

    return nodeToIndex;
}

// Checks whether the paths between two pairs of leaves are edge-disjoint.
// Computes both paths via getPath and checks whether their intersection is empty.
// Returns true if the paths share no common edges, false otherwise.
bool areTwoPathsDisjoint(const graph::Forest& forest,
                          unsigned int lpair1, unsigned int rpair1,
                          unsigned int lpair2, unsigned int rpair2,
                          cluster::LeastCommonAncestor& lca,
                          const std::unordered_map<const graph::Node*, int>& nodeToIndex)
{
    // All Labels should be different from each other...
    assert(lpair1 != rpair1);
    assert(lpair2 != rpair2);
    assert(lpair1 != lpair2);
    assert(lpair1 != rpair2);
    assert(rpair1 != lpair2);
    assert(rpair1 != rpair2);

    // Idea: Calculate both Paths between Pairs and checker wether intersection of paths is empty...
    std::set<int> path1 = getPath(forest, lpair1, rpair1, lca, nodeToIndex);
    std::set<int> path2 = getPath(forest, lpair2, rpair2, lca, nodeToIndex);

    // Paths should not be empty...
    assert(!path1.empty());
    assert(!path2.empty());

    std::set<int> intersection;
    std::set_intersection(path1.begin(), path1.end(),
                          path2.begin(), path2.end(),
                          std::inserter(intersection, intersection.begin()));

    return intersection.empty();
}

// Computes the set of edge indices on the path between two leaves.
// Uses the LCA to find the meeting point of both leaves.
// Traverses upwards from each leaf to the LCA, collecting edge indices.
// The LCA node itself is not included as it represents no edge on the path.
std::set<int> getPath(const graph::Forest& forest,
                      unsigned int leaf1, unsigned int leaf2,
                      cluster::LeastCommonAncestor& lca,
                      const std::unordered_map<const graph::Node*, int>& nodeToIndex)
{
    // Labels should exist and be unique...
    assert(forest.LabelToTerminal().count(leaf1) > 0);
    assert(forest.LabelToTerminal().count(leaf2) > 0);
    assert(leaf1 != leaf2);

    // Get LCA...
    const graph::Node* lcaNode = lca.getLeastCommonAncestor(
        forest.LabelToTerminal().at(leaf1),
        forest.LabelToTerminal().at(leaf2)
    );

    // LCA should exist...
    assert(lcaNode != nullptr);

    std::set<int> edges;

    // Path from leaf1 to LCA...
    const graph::Node* current = forest.LabelToTerminal().at(leaf1);
    while(current != lcaNode)
    {   
        // All Labels on Path should exist...
        assert(nodeToIndex.count(current) > 0);
        edges.insert(nodeToIndex.at(current));
        current = current->parent;
    }

    // Path from leaf2 to LCA...
    current = forest.LabelToTerminal().at(leaf2);
    while(current != lcaNode)
    {
        // All Labels on Path should exist...
        assert(nodeToIndex.count(current) > 0);
        edges.insert(nodeToIndex.at(current));
        current = current->parent;
    }

    // Set of edge indices should not be empty...
    assert(!edges.empty());
    return edges;
}

}  // namespace solver