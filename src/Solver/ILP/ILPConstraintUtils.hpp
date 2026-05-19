#ifndef PACE2026_ILPCONSTRAINT_UTILS_HPP
#define PACE2026_ILPCONSTRAINT_UTILS_HPP

#include "../../Graph/Forest.hpp"
#include "../../Graph/Node.hpp"
#include "../../Cluster/LeastCommonAncestor.hpp"

#include <unordered_map>
#include <memory>
#include <vector>
#include <set>

namespace solver {

/// \brief Computes the triple constraints from the two trees.
/// \param forest1 First Tree (Forest) of binary MAF-Problem.
/// \param forest2 Second Tree (Forest) of binary MAF-Problem.
/// \param lca1 Precomputed LCA table for forest1.
/// \param lca2 Precomputed LCA table for forest2.
/// \param nodeToIndex1 IndexMap of first Forest to extract indices of nodes / edges.
/// \param nodeToIndex2 IndexMap of second Forest to extract indices of nodes / edges.
/// \return Set of all triple constraints.
/// \note A triple constraint is a set of edges whose removal resolves
///       an incompatible triple between the two trees.
[[nodiscard]]
std::vector<std::vector<int>> computeTripleConstraints(
                const graph::Forest& forest1,
                const graph::Forest& forest2,
                cluster::LeastCommonAncestor& lca1,
                cluster::LeastCommonAncestor& lca2,
                const std::unordered_map<const graph::Node*, int>& nodeToIndex1,
                const std::unordered_map<const graph::Node*, int>& nodeToIndex2,
                std::unordered_map<uint64_t, std::vector<int>>& pathCache);

/// \brief Computes path-pair constraints from the two trees.
/// \param forest1 First Tree (Forest) of binary MAF-Problem.
/// \param forest2 Second Tree (Forest) of binary MAF-Problem.
/// \param lca1 Precomputed LCA table for forest1.
/// \param lca2 Precomputed LCA table for forest2.
/// \param nodeToIndex1 IndexMap of first Forest to extract indices of nodes / edges.
/// \param nodeToIndex2 IndexMap of second Forest to extract indices of nodes / edges.
/// \return Set of all path-pair constraints.
/// \note A path-pair constraint is a set of edges whose removal resolves
///       an incompatible pair of disjoint paths between the two trees.
[[nodiscard]]
std::vector<std::vector<int>> computePathPairConstraints(
                const graph::Forest& forest1,
                const graph::Forest& forest2,
                cluster::LeastCommonAncestor& lca1,
                cluster::LeastCommonAncestor& lca2,
                const std::unordered_map<const graph::Node*, int>& nodeToIndex1,
                const std::unordered_map<const graph::Node*, int>& nodeToIndex2,
                std::unordered_map<uint64_t, std::vector<int>>& pathCache1,
                std::unordered_map<uint64_t, std::vector<int>>& pathCache2);

} // namespace solver

#endif // PACE2026_ILPCONSTRAINT_UTILS_HPP