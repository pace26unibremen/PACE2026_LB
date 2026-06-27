#ifndef PACE2026_ILPFORMULATION_HPP
#define PACE2026_ILPFORMULATION_HPP

#include "../../Graph/Forest.hpp"
#include "../../Graph/Node.hpp"
#include "../Cluster/LeastCommonAncestor.hpp"
#include "ILPModel.hpp"

#include <memory>

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