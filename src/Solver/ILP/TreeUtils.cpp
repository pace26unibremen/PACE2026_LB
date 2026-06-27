#include "TreeUtils.hpp"

#include <cassert>
#include <stdexcept>
#include <algorithm>
#include<iostream>

namespace solver {

void recBuildLabelToTerminal(graph::Node* const& n, std::unordered_map<unsigned int, graph::Node*>& labelToTerminal, const graph::Forest& forest)
{
    if (!n) return;
    if (!n->leftChild)  // Blatt = kein linkes Kind
        labelToTerminal[*n->SubtreeLabels().begin()] = n;
    recBuildLabelToTerminal(n->leftChild, labelToTerminal, forest);
    recBuildLabelToTerminal(n->rightChild, labelToTerminal, forest);
}

std::unordered_map<unsigned int, graph::Node*> buildLabelToTerminal(const graph::Forest& forest)
{
    auto labelToTerminal = std::unordered_map<unsigned int, graph::Node*>();
    for (const auto& r : forest.Roots())
        recBuildLabelToTerminal(r, labelToTerminal, forest);
    return labelToTerminal;
}

std::unordered_map<graph::Node*, unsigned int> buildTerminalToLabel(const graph::Forest& forest)
{
    auto terminalToLabel = std::unordered_map<graph::Node*, unsigned int>();
    auto labelToTerminal = buildLabelToTerminal(forest);
    for (const auto& [label, node] : labelToTerminal)
        (terminalToLabel)[node] = label;
    return terminalToLabel;
}

int getRootIndex(graph::Forest& forest)
{
    // This Function should only be called, when forest is not yet split up...
    assert(forest.Roots().size() == 1);
    std::unordered_map<const graph::Node*, int> nodeToIndex = buildNodeToIndexMap(forest);
    return nodeToIndex.at(forest.Roots()[0]);
}

int getLCA(unsigned int leaf1, unsigned int leaf2,
           const graph::Forest& forest,
           cluster::LeastCommonAncestor& lca,
           const std::unordered_map<const graph::Node*, int>& nodeToIndex)
{
    auto labelToTerminal = buildLabelToTerminal(forest);
    // std::cout << "getLCA(): Labels: (" << leaf1 << "," << leaf2 << ")" << std::endl;

    graph::Node* node1 = labelToTerminal.at(leaf1);
    graph::Node* node2 = labelToTerminal.at(leaf2);

    // Compute LCA of Nodes...
    graph::Node* lcaNode = lca.getLeastCommonAncestor(node1, node2);

    // Return Index...
    assert(nodeToIndex.count(lcaNode) > 0);
    return nodeToIndex.at(lcaNode);
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


// Auxiliary recursive Function for DFS of the forest.
void recBuildIndexToNodeMap(graph::Node* & n, std::unordered_map<int, graph::Node*> & indexToNode, int &i)
{
    if (!n) return;
    indexToNode[i++] = n;

    recBuildIndexToNodeMap(n->leftChild, indexToNode, i);
    recBuildIndexToNodeMap(n->rightChild, indexToNode, i);
}

std::unordered_map<int, graph::Node*> buildIndexToNodeMap(graph::Forest& forest)
{
    std::unordered_map<int, graph::Node*> indexToNode;
    int i = 0;

    for (auto& r : forest.Roots())
    {
       recBuildIndexToNodeMap(r, indexToNode, i);
    }

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
std::vector<int> getPath(const graph::Forest& forest,
                      unsigned int leaf1, unsigned int leaf2,
                      cluster::LeastCommonAncestor& lca,
                      const std::unordered_map<const graph::Node*, int>& nodeToIndex)
{ 
    std::vector<int> edges;
    if (leaf1 == leaf2)
        return edges;

    auto labelToTerminal = buildLabelToTerminal(forest);
    // Get LCA...
    const graph::Node* lcaNode = lca.getLeastCommonAncestor(
        labelToTerminal.at(leaf1),
        labelToTerminal.at(leaf2)
    );

    // LCA should exist...
    assert(lcaNode != nullptr);

    // Path from leaf1 to LCA...
    const graph::Node* current = labelToTerminal.at(leaf1);
    while(current != lcaNode)
    {   
        assert(nodeToIndex.count(current) > 0);
        edges.push_back(nodeToIndex.at(current));
        current = current->parent;
    }

    // Path from leaf2 to LCA...
    current = labelToTerminal.at(leaf2);
    while(current != lcaNode)
    {
        assert(nodeToIndex.count(current) > 0);
        edges.push_back(nodeToIndex.at(current));
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