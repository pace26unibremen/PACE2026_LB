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
        /// \brief Data of First Tree (Forest) of binary MAF-Problem.
        ForestData forestData_1;
        /// \brief Data of Second Tree (Forest) of binary MAF-Problem.
        ForestData forestData_2;

        unsigned int rootLabel = 0;

    public:
        /// \brief Constructor of ILPFormulation.
        /// \param forest1 First tree of the instance.
        /// \param forest2 Second tree of the instance.
        ILPFormulation(std::shared_ptr<graph::Forest> forest1, std::shared_ptr<graph::Forest> forest2, unsigned int rootLabel = 0);
    
        /// \brief Builds the ILP problem from the two trees.
        /// \returns The ILP problem ready to be passed to a solver.
        [[nodiscard]]
        ILPProblem build() const;

};

} // namespace solver

#endif  // PACE2026_ILPFORMULATION_HPP