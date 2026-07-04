#include "TreeUtils.hpp"

#include <cassert>
#include <stdexcept>
#include <algorithm>
#include<iostream>

namespace solver {


std::unordered_map<unsigned int, graph::Node*> buildLabelToTerminal(const graph::Forest& forest)
{
    auto labelToTerminal = std::unordered_map<unsigned int, graph::Node*>();
    for (auto& [node, label] : forest.TerminalToLabel())
        (labelToTerminal)[label] = node;
    return labelToTerminal;
}

int getRootIndex(graph::Forest& forest)
{
    // This Function should only be called, when forest is not yet split up...
    if (forest.Roots().empty())
        throw std::logic_error("getRootIndex: forest has no root node");
    assert(forest.Roots().size() == 1);
    std::unordered_map<const graph::Node*, int> nodeToIndex = buildNodeToIndexMap(forest);
    return nodeToIndex.at(forest.Roots()[0]);
}

int getLCA(unsigned int leaf1, unsigned int leaf2,
           const ForestData& forestData)
{
    graph::Node* node1 = forestData.labelToTerminal.at(leaf1);
    graph::Node* node2 = forestData.labelToTerminal.at(leaf2);

    // Compute LCA of Nodes...
    graph::Node* lcaNode = (*forestData.lca).getLeastCommonAncestor(node1, node2);

    // Return Index...
    assert(forestData.nodeToIndex.count(lcaNode) > 0);
    return forestData.nodeToIndex.at(lcaNode);
}

// Auxiliary recursive Function for DFS of the forest.
void recBuildNodeToIndexMap(graph::Node* const & n, std::unordered_map<const graph::Node*, int> & nodeToIndex, int &i)
{
    if (!n) return;
    nodeToIndex[n] = i++;

    recBuildNodeToIndexMap(n->leftChild, nodeToIndex, i);
    recBuildNodeToIndexMap(n->rightChild, nodeToIndex, i);
}

std::unordered_map<const graph::Node*, int> buildNodeToIndexMap(const graph::Forest& forest)
{
    std::unordered_map<const graph::Node*, int> nodeToIndex;
    int i = 0;

    for (const auto& r : forest.Roots())
    {
       recBuildNodeToIndexMap(r, nodeToIndex, i);
    }

    return nodeToIndex;
}

std::unordered_map<int, graph::Node*> buildIndexToNodeMap(graph::Forest& forest)
{
    auto indexToNode = std::unordered_map<int, graph::Node*>();
    auto nodeToIndex = buildNodeToIndexMap(forest);
    
    for (auto& [node, label] : nodeToIndex)
        (indexToNode)[label] = const_cast<graph::Node*>(node);
    return indexToNode;
}

// Auxiliary recursive Function for DFS of the forest.
void recGetNumVars(graph::Node* const & n, int & i)
{
    if (!n) return;

    i++;
    recGetNumVars(n->leftChild, i);
    recGetNumVars(n->rightChild, i);
}

int getNumVars(const graph::Forest& forest)
{
    int i = 0;
    for (const auto& r : forest.Roots())
    {
        recGetNumVars(r, i);
    }

    return i;
}


// Checks whether the paths between two pairs of leaves are edge-disjoint.
// Computes both paths via getPath and checks whether their intersection is empty.
// Returns true if the paths share no common edges, false otherwise.
bool areTwoPathsDisjoint(unsigned int lpair1, unsigned int rpair1,
                         unsigned int lpair2, unsigned int rpair2,
                         const ForestData& forestData)
{
    // Idea: Calculate both Paths between pairs and check wether intersection of paths is empty...
    std::vector<int> path1 = getPath(lpair1, rpair1, forestData);
    std::vector<int> path2 = getPath(lpair2, rpair2, forestData);

    // If one path is empty throw logic_error...
    if (path1.empty() || path2.empty())
    {
        std::cout << "TreeUtils::areTwoPathsDisjoint: at least one path is empty! \n" <<
            "Pair 1: " << lpair1 << "," << rpair1 << "\n" <<
            "Pair 2: " << lpair2 << "," << rpair2 << std::endl; 
        throw std::logic_error("TreeUtils::areTwoPathsDisjoint: at least one path is empty!");
    }
    bool disjoint = !std::any_of(path1.begin(), path1.end(), [&](int x) {
        return std::binary_search(path2.begin(), path2.end(), x);
    }); 

    return disjoint;
}

// Computes the set of edge indices on the path between two leaves.
// Uses the LCA to find the meeting point of both leaves.
// Traverses upwards from each leaf to the LCA, collecting edge indices.
// The LCA node itself is not included as it represents no edge on the path.
std::vector<int> getPath(unsigned int leaf1, unsigned int leaf2, const ForestData& forestData)
{ 
    // Calculate Key for Cache...
    if (leaf1 > leaf2) std::swap(leaf1, leaf2);
    uint64_t key = (static_cast<uint64_t>(leaf1) << 32) | leaf2;

    // Look up in Cache...
    auto it = forestData.pathCache.find(key);
    if (it != forestData.pathCache.end())
        return it->second;

    std::vector<int> edges;
    
    // Get LCA...
    const graph::Node* lcaNode = (*forestData.lca).getLeastCommonAncestor(
        forestData.labelToTerminal.at(leaf1),
        forestData.labelToTerminal.at(leaf2)
    );

    // LCA should exist...
    assert(lcaNode != nullptr);

    // Path from leaf1 to LCA...
    const graph::Node* current = forestData.labelToTerminal.at(leaf1);
    while(current != lcaNode)
    {   
        assert(forestData.nodeToIndex.count(current) > 0);
        edges.push_back(forestData.nodeToIndex.at(current));
        current = current->parent;
    }

    // Path from leaf2 to LCA...
    current = forestData.labelToTerminal.at(leaf2);
    while(current != lcaNode)
    {
        assert(forestData.nodeToIndex.count(current) > 0);
        edges.push_back(forestData.nodeToIndex.at(current));
        current = current->parent;
    }

    // Set of edge indices should not be empty...
    assert(!edges.empty());
    if(edges.empty())
    {
        std::cout << "TreeUtils::getPath: path is empty! \n" <<
            "Leaf 1: " << leaf1 << ", Leaf 2: " << leaf2 << "\n" << std::endl; 
        throw std::logic_error("TreeUtils::getPath: path is empty!"); 
    }
    // Sort edges...
    std::sort(edges.begin(), edges.end());
    auto [inserted_it, _] = forestData.pathCache.emplace(key, std::move(edges));

    return inserted_it->second;
}

bool checkIncompatibleTriple(unsigned int leaf1, unsigned int leaf2, unsigned int leaf3,
                             const ForestData& forestData1, const ForestData& forestData2)
{   

    // If two leaves are equal throw logic_error...
    if (leaf1 == leaf2 || leaf1 == leaf3 || leaf2 == leaf3)
        throw std::logic_error("TreeUtils::checkIncompatibleTriple: at least two leafs are equal");

    bool fIncTriple = false;

    int lcaij1 = getLCA(leaf1, leaf2, forestData1);
    int lcajk1 = getLCA(leaf2, leaf3, forestData1);
    int lcaik1 = getLCA(leaf1, leaf3, forestData1);
    int lcaij2 = getLCA(leaf1, leaf2, forestData2);
    int lcajk2 = getLCA(leaf2, leaf3, forestData2);
    int lcaik2 = getLCA(leaf1, leaf3, forestData2);

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