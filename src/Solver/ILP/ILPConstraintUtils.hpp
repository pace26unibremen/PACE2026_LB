#ifndef PACE2026_ILPCONSTRAINT_UTILS_HPP
#define PACE2026_ILPCONSTRAINT_UTILS_HPP

#include "../../Graph/Forest.hpp"
#include "../../Graph/Node.hpp"
#include "../Cluster/LeastCommonAncestor.hpp"
#include "ILPModel.hpp"

#include <unordered_map>
#include <memory>
#include <vector>
#include <set>

namespace solver {

/// \brief Computes the triple constraints from the two trees.
/// \param forestData_1 Data Object of the forest 1.
/// \param forestData_2 Data Object of the forest 2.
/// \return Set of all triple constraints.
/// \note A triple constraint is a set of edges whose removal resolves
///       an incompatible triple between the two trees.
[[nodiscard]]
std::vector<std::vector<int>> computeTripleConstraints(const ForestData& forestData_1, const ForestData& forestData_2);

/// \brief Computes path-pair constraints from the two trees.
/// \param forestData_1 Data Object of the forest 1.
/// \param forestData_2 Data Object of the forest 2.
/// \return Set of all path-pair constraints.
/// \note A path-pair constraint is a set of edges whose removal resolves
///       an incompatible pair of disjoint paths between the two trees.
[[nodiscard]]
std::vector<std::vector<int>> computePathPairConstraints(const ForestData& forestData_1, const ForestData& forestData_2);

} // namespace solver

#endif // PACE2026_ILPCONSTRAINT_UTILS_HPP