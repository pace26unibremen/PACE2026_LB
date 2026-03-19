#ifndef PACE2026_TREE_UTILS_HPP
#define PACE2026_TREE_UTILS_HPP

#include "../../Graph/Forest.hpp"
#include "../../Graph/Node.hpp"
#include "../../Cluster/LeastCommonAncestor.hpp"

#include <unordered_map>
#include <memory>
#include <set>

namespace solver {

/// \brief Returns the index of the root node in Forest::Nodes().
/// \note Root is identified as the node with parent == nullptr.
/// \param forest The forest to search in.
/// \returns Index of the root node in Forest::Nodes().
/// \throws std::logic_error if no root node is found.
/// \assert Exactly one root node exists (parent == nullptr).
int getRootIndex(std::shared_ptr<graph::Forest> forest);

/// \brief Builds a map from Node pointer to index in Forest::Nodes().
/// \note Only valid as long as the Forest is not modified.
/// \param forest Forest on which the map is built.
/// \return Map of node index <-> node* in Forest.
std::unordered_map<const graph::Node*, int> buildNodeToIndexMap(const graph::Forest& forest);

/// \brief Returns the index of the Most Recent Common Ancestor of two leaves.
/// \param forest Forest, that the leaves belong to.
/// \param lca Precomputed LCA table for Forest.
/// \param leaf1 First Leaf.
/// \param leaf2 Second Leaf.
/// \param nodeToIndex Map that helps extract the Index of common ancestor.
/// \return Most Recent Common Ancestor of both Leaves.
int getLCA(const graph::Forest& forest, const cluster::LeastCommonAncestor& lca, int leaf1, unsigned int leaf2,
            const std::unordered_map<const graph::Node*, int>& nodeToIndex);

/// \brief Returns the set of edge indices on the path between two leaves.
/// \param forest Forest, that the leaves belong to.
/// \param lca Precomputed LCA table for Forest.
/// \param leaf1 First Leaf.
/// \param leaf2 Second Leaf.
/// \param nodeToIndex Map that helps extract the Index of edges.
/// \return Set of edge indices.
std::set<int> getPath(const graph::Forest& forest,
                      unsigned int leaf1, unsigned int leaf2,
                      cluster::LeastCommonAncestor& lca,
                      const std::unordered_map<const graph::Node*, int>& nodeToIndex);

/// \brief Checks whether two paths between leaf pairs are disjoint.
/// \param forest Forest, that the leaves belong to.
/// \param lca Precomputed LCA table for Forest.
/// \param lpair1 First Leaf of first (left) pair.
/// \param rpair1 First Leaf of second (right) pair.
/// \param lpair2 Second Leaf of first (left) pair.
/// \param rpair2 Second Leaf of second (right) pair.
/// \param nodeToIndex Map that helps extract the indices of edges.
/// \return true if paths of leaf pairs are disjoint, false otherwise.
bool areTwoPathsDisjoint(const graph::Forest& forest,
                          unsigned int lpair1, unsigned int rpair1,
                          unsigned int lpair2, unsigned int rpair2,
                          cluster::LeastCommonAncestor& lca,
                          const std::unordered_map<const graph::Node*, int>& nodeToIndex);

}  // namespace solver

#endif  // PACE2026_TREE_UTILS_HPP