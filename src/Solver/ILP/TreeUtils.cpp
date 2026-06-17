#include "TreeUtils.hpp"

#include <cassert>
#include <stdexcept>
#include <algorithm>
#include<iostream>

namespace solver {

int getRootIndex(graph::Forest& forest)
{
    int rootIndex = -1;
    int rootCount = 0;
    int nNodes = forest.Nodes().size();
    for(int i = 0; i < nNodes; ++i)
    {
        if(forest.Nodes()[i].parent == nullptr)
        {
            rootIndex = i;
            rootCount++;
        }
    }

    // Exactly one root node must exist
    if(rootCount > 1)
        throw std::logic_error("TreeUtils::getRootIndex: multiple roots found");

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
    //assert(forest.LabelToTerminal().count(leaf1) > 0);
    //assert(forest.LabelToTerminal().count(leaf2) > 0);

    graph::Node* node1 = forest.LabelToTerminal().at(leaf1);
    graph::Node* node2 = forest.LabelToTerminal().at(leaf2);

    /*std::cout << "Forest adresse: " << &forest << std::endl;
    std::cout << "LCA adresse: " << &lca << std::endl;
    std::cout << "node1: " << node1 << std::endl;
    std::cout << "node2: " << node2 << std::endl; 
*/
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

std::unordered_map<int, graph::Node*> buildIndexToNodeMap(graph::Forest& forest)
{
    std::unordered_map<int, graph::Node*> indexToNode;

    for(int i = 0; i < (int)forest.Nodes().size(); ++i)
        indexToNode[i] = &forest.Nodes()[i];

    // Each index should have a unique node...
    assert(indexToNode.size() == forest.Nodes().size());

    return indexToNode;
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

    // Idea: Calculate both Paths between pairs and check wether intersection of paths is empty...
    std::vector<int> path1 = getPath(forest, lpair1, rpair1, lca, nodeToIndex);
    std::vector<int> path2 = getPath(forest, lpair2, rpair2, lca, nodeToIndex);

    // If one path is empty throw logic_error...
    if (path1.empty() || path2.empty())
        throw std::logic_error("TreeUtils::areTwoPathsDisjoint: at least one path is empty!");

    bool disjoint = !std::any_of(path1.begin(), path1.end(), [&](int x) {
        return std::binary_search(path2.begin(), path2.end(), x);
    }); 

    return disjoint;
}

// Computes the set of edge indices on the path between two leaves.
// Uses the LCA to find the meeting point of both leaves.
// Traverses upwards from each leaf to the LCA, collecting edge indices.
// The LCA node itself is not included as it represents no edge on the path.
std::vector<int> getPath(const graph::Forest& forest,
                      unsigned int leaf1, unsigned int leaf2,
                      cluster::LeastCommonAncestor& lca,
                      const std::unordered_map<const graph::Node*, int>& nodeToIndex)
{
    // If path is not already in cache, calculate new...
    std::vector<int> edges;
    if (leaf1 == leaf2)
        return edges;

    // Get LCA...
    const graph::Node* lcaNode = lca.getLeastCommonAncestor(
        forest.LabelToTerminal().at(leaf1),
        forest.LabelToTerminal().at(leaf2)
    );

    // LCA should exist...
    assert(lcaNode != nullptr);

    // Path from leaf1 to LCA...
    const graph::Node* current = forest.LabelToTerminal().at(leaf1);
    while(current != lcaNode)
    {   
        assert(nodeToIndex.count(current) > 0);
        edges.push_back(nodeToIndex.at(current));
        current = current->parent;
    }

    // Path from leaf2 to LCA...
    current = forest.LabelToTerminal().at(leaf2);
    while(current != lcaNode)
    {
        assert(nodeToIndex.count(current) > 0);
        edges.push_back(nodeToIndex.at(current));
        current = current->parent;
    }

    // Set of edge indices should not be empty...
    assert(!edges.empty());

    // Sort edges...
    std::sort(edges.begin(), edges.end());
    return edges;
}

bool checkIncompatibleTriple(unsigned int leaf1, unsigned int leaf2, unsigned int leaf3,
                        const graph::Forest& forest1,
                        const graph::Forest& forest2,
                        cluster::LeastCommonAncestor& lca1,
                        cluster::LeastCommonAncestor& lca2, 
                        const std::unordered_map<const graph::Node*, int>& nodeToIndex1,
                        const std::unordered_map<const graph::Node*, int>& nodeToIndex2)
{   
    // Forests or LCAs should not be nullpointers...
    //assert(forest1.isValid());
    //assert(forest2.isValid());

    // Labels should exist in both forests...
    //assert(forest1.LabelToTerminal().count(leaf1) > 0);
    //assert(forest1.LabelToTerminal().count(leaf2) > 0);
    //assert(forest1.LabelToTerminal().count(leaf3) > 0);
    //assert(forest2.LabelToTerminal().count(leaf1) > 0);
    //assert(forest2.LabelToTerminal().count(leaf2) > 0);
    //assert(forest2.LabelToTerminal().count(leaf3) > 0);

    // If two leaves are equal throw logic_error...
    if (leaf1 == leaf2 || leaf1 == leaf3 || leaf2 == leaf3)
        throw std::logic_error("TreeUtils::checkIncompatibleTriple: at least two leafs are equal");

    bool fIncTriple = false;

    int lcaij1 = getLCA(leaf1, leaf2, forest1, lca1, nodeToIndex1);
    int lcajk1 = getLCA(leaf2, leaf3, forest1, lca1, nodeToIndex1);
    int lcaik1 = getLCA(leaf1, leaf3, forest1, lca1, nodeToIndex1);
    int lcaij2 = getLCA(leaf1, leaf2, forest2, lca2, nodeToIndex2);
    int lcajk2 = getLCA(leaf2, leaf3, forest2, lca2, nodeToIndex2);
    int lcaik2 = getLCA(leaf1, leaf3, forest2, lca2, nodeToIndex2);

    if(lcaij1 == lcajk1)
    {
        if(lcaij2 != lcajk2) fIncTriple = true;
    }
    else if(lcaij1 == lcaik1)
    {
        if(lcaij2 != lcaik2) fIncTriple = true;
    }
    else
    {
        if(lcaik2 != lcajk2) fIncTriple = true;
    }

    return fIncTriple;
}

}  // namespace solver