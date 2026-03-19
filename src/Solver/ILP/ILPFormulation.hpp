#ifndef PACE2026_ILPFORMULATION_HPP
#define PACE2026_ILPFORMULATION_HPP

#include "../../Graph/Forest.hpp"
#include "../../Graph/Node.hpp"
#include "../../Cluster/LeastCommonAncestor.hpp"
#include "ILPModel.hpp"
#include "TreeUtils.hpp"

#include <memory>
#include <unordered_map>
#include <vector>
#include <set>

namespace solver {

/// \brief Builds the ILP formulation for the MAF problem.
/// \note Tree reductions are NOT part of this formulation and should be
///       applied before calling build(). This is intentional to keep the
///       formulation independent of the reduction pipeline.
class ILPFormulation 
{
    private:
        /// \brief First Tree (Forest) of binary MAF-Problem.
        std::shared_ptr<graph::Forest> forest1;

        /// \brief Second Tree (Forest) of binary MAF-Problem.
        std::shared_ptr<graph::Forest> forest2;

        /// \brief Precomputed LCA table for forest1.
        /// \note Must be reinstantiated if forest1 changes.
        std::shared_ptr<cluster::LeastCommonAncestor> lca1;
 
        /// \brief Precomputed LCA table for forest2.
        /// \note Must be reinstantiated if forest2 changes.
        std::shared_ptr<cluster::LeastCommonAncestor> lca2;

        /// \brief Computes the triple constraints from the two trees.
        /// \param nodeToIndex1 IndexMap of first Forest to extract indices of nodes / edges.
        /// \param nodeToIndex2 IndexMap of second Forest to extract indices of nodes / edges.
        /// \return Set of all triple constraints.
        /// \note A triple constraint is a set of edges whose removal resolves
        ///       an incompatible triple between the two trees.
        [[nodiscard]]
        std::vector<std::set<int>> computeTripleConstraints(
                        const std::unordered_map<const graph::Node*, int>& nodeToIndex1,
                        const std::unordered_map<const graph::Node*, int>& nodeToIndex2) const;

        /// \brief Computes path-pair constraints from the two trees.
        /// \param nodeToIndex1 IndexMap of first Forest to extract indices of nodes / edges.
        /// \param nodeToIndex2 IndexMap of second Forest to extract indices of nodes / edges.
        /// \return Set of all path-pair constraints.
        /// \note A path-pair constraint is a set of edges whose removal resolves
        ///       an incompatible pair of disjoint paths between the two trees.
        [[nodiscard]]
        std::vector<std::set<int>> computePathPairConstraints(
                        const std::unordered_map<const graph::Node*, int>& nodeToIndex1,
                        const std::unordered_map<const graph::Node*, int>& nodeToIndex2) const;
        
        /// \brief Checks wether Triple of Leaves is topologically incompatible.
        /// \param forest1 First Forest.
        /// \param forest2 Second Forest.
        /// \param leaf1 Index of First Leaf.
        /// \param leaf2 Index of Second Leaf.
        /// \param leaf3 Index of Third Leaf.
        /// \param nodeToIndex1 Map that helps translating indices to nodes of first forest.
        /// \param nodeToIndex2 Map that helps translating indices to nodes of second forest.
        /// \return true, if triple is incompatible, false otherwise
        [[nodiscard]]
        bool checkIncompatibleTriple(unsigned int leaf1, unsigned int leaf2, unsigned int leaf3, 
                                const std::unordered_map<const graph::Node*, int>& nodeToIndex1,
                                const std::unordered_map<const graph::Node*, int>& nodeToIndex2) const;


    public:
        /// \brief Constructor of ILPFormulation.
        /// \param forest1 First tree of the instance.
        /// \param forest2 Second tree of the instance.
        ILPFormulation(std::shared_ptr<graph::Forest> forest1, std::shared_ptr<graph::Forest> forest2);
    
        /// \brief Builds the ILP problem from the two trees.
        /// \returns The ILP problem ready to be passed to a solver.
        [[nodiscard]]
        ILPProblem build() const;

};

} // namespace solver

#endif  // PACE2026_ILPFORMULATION_HPP