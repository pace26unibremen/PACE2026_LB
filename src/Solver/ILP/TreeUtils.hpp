#ifndef PACE2026_TREE_UTILS_HPP
#define PACE2026_TREE_UTILS_HPP

#include "../../Graph/Forest.hpp"
#include "../../Graph/Node.hpp"
#include "../Cluster/LeastCommonAncestor.hpp"
#include "ILPModel.hpp"

#include <unordered_map>
#include <memory>
#include <set>

namespace solver {

/// \brief Returns the index of the root node in Forest::Nodes().
/// \note Root is identified as the node with parent == nullptr.
/// \param forest The forest to search in.
/// \returns Index of the root node in Forest::Nodes().
/// \throws std::logic_error if no root node is found.
int getRootIndex(graph::Forest& forest);

/// \brief Builds an updated map, that stores all leaf labels with corresponding terminal Pointers.
/// \param forest the (reduced) forest. 
/// \returns Pointer to the new map...
std::unordered_map<unsigned int, graph::Node*> buildLabelToTerminal(const graph::Forest& forest);

/// \brief Builds a map from Node pointer to index in Forest::Nodes().
/// \note Only valid as long as the Forest is not modified.
/// \param forest Forest on which the map is built.
/// \return Map of node* <-> node index in Forest.
std::unordered_map<const graph::Node*, int> buildNodeToIndexMap(const graph::Forest& forest);

/// \brief Builds a map from index to Node pointer in Forest::Nodes().
/// \note Not exact inverse of buildNodeToIndexMap as non-constant nodes are needed for DeleteEdgeAction...
/// \note Only valid as long as the Forest is not modified.
/// \param forest Forest on which the map is built.
/// \return Map of node index <-> node* in Forest.
std::unordered_map<int, graph::Node*> buildIndexToNodeMap(graph::Forest& forest);

/// \brief Gives back the number of current nodes in the forest, used as Variables in the ILP FOrmulation.
/// \param forest Forest.
/// \return Number of current Nodes, representing vertices.
int getNumVars(const graph::Forest& forest);

/// \brief Returns the index of the Most Recent Common Ancestor of two leaves.
/// \param leaf1 First Leaf.
/// \param leaf2 Second Leaf.
/// \param forestData Data Object of the forest.
/// \return Most Recent Common Ancestor of both Leaves.
int getLCA(int leaf1, unsigned int leaf2, const ForestData& forestData);

/// \brief Returns the set of edge indices on the path between two leaves.
/// \param leaf1 Label of first Leaf.
/// \param leaf2 Label of second Leaf.
/// \param forestData Data Object of the forest.
/// \note Edge Indizes are different to the labels of leafs!!!
/// \return Set of edge indices. (empty set if leaf1 == leaf2)
std::vector<int> getPath(unsigned int leaf1, unsigned int leaf2,
                         const ForestData& forestData);

/// \brief Checks whether two paths between leaf pairs are disjoint.
/// \param lpair1 First Leaf of first (left) pair.
/// \param rpair1 First Leaf of second (right) pair.
/// \param lpair2 Second Leaf of first (left) pair.
/// \param rpair2 Second Leaf of second (right) pair.
/// \param forestData Data Object of the forest.
/// \return true if paths of leaf pairs are disjoint, false otherwise.
bool areTwoPathsDisjoint(unsigned int lpair1, unsigned int rpair1,
                         unsigned int lpair2, unsigned int rpair2,
                         const ForestData& forestData);

/// \brief Checks wether Triple of Leaves is topologically incompatible between two trees.
/// \param leaf1 Index of First Leaf.
/// \param leaf2 Index of Second Leaf.
/// \param leaf3 Index of Third Leaf.
/// \param forestData1 Data Object of the forest 1.
/// \param forestData2 Data Object of the forest 2.
/// \return true, if triple is incompatible, false otherwise
[[nodiscard]]
bool checkIncompatibleTriple(unsigned int leaf1, unsigned int leaf2, unsigned int leaf3,
                            const ForestData& forestData1,
                            const ForestData& forestData2);


}  // namespace solver

#endif  // PACE2026_TREE_UTILS_HPP