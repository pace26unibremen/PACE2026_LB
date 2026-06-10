#ifndef PACE2026_INCREMENTALILPSOLVER_HPP
#define PACE2026_INCREMENTALILPSOLVER_HPP

#include "AbstractIncrementalSolver.hpp"
#include "../Action/DeleteEdgeAction.hpp"
#include "../../Cluster/LeastCommonAncestor.hpp"
#include "../../Graph/Forest.hpp"
#include "../../Graph/ForestIO.hpp"
#include "../../Graph/Instance.hpp"
#include "../AbstractSolver.hpp"
#include "ILPModel.hpp"

#include <vector>
#include <memory>

namespace solver {

/// \brief Available MaxSAT solver.
enum class MaxSATSolverType {
    // EvalMaxSAT,
    UWrMaxSat
};

class IncrementalMAFSolver : public AbstractSolver
{
    static constexpr int MAX_ROUNDS = 1000;
    private:

    std::unique_ptr<AbstractIncrementalSolver> solver;
    std::shared_ptr<graph::Forest> forest_1;
    std::shared_ptr<graph::Forest> forest_2;
    std::unordered_map<const graph::Node*, int> nodeToIndex1;
    std::unordered_map<const graph::Node*, int> nodeToIndex2;
    std::vector<int> cutEdges;
    /// \brief Precomputed LCA table for forest1.
    /// \note Must be reinstantiated if forest1 changes.
    std::shared_ptr<cluster::LeastCommonAncestor> lca1;

    /// \brief Precomputed LCA table for forest2.
    /// \note Must be reinstantiated if forest2 changes.
    std::shared_ptr<cluster::LeastCommonAncestor> lca2;


    /// \brief Cache for getPath() function for forest1...
    mutable std::unordered_map<uint64_t, std::vector<int>> pathCache1;

    /// \brief Cache for getPath() function for forest2...
    mutable std::unordered_map<uint64_t, std::vector<int>> pathCache2;

    /// \brief Creates the concrete ILP solver based on solverType.
    /// \param solverType The solver type to create.
    void buildSolver(MaxSATSolverType solverType);

    /// \brief Adds initial constraints to solver.
    void addInitialConstraints();

    /// \brief Generates Constraints from Violations of MAF-Check...
    void generateConstraints(std::vector<const graph::Node*> subtreeRoots,
                                const std::shared_ptr<graph::Forest>& mafSolution);

    /// \brief Extracts cut edges from a solution.
    /// \param solution The solution returned by the solver.
    void extractCutEdges(const ILPSolution& ilpSolution);

    /// \brief Reconstructs the MAF from cut edges.
    /// \param cutEdges The cut edges.
    /// \return 
    [[nodiscard]]
    std::shared_ptr<graph::Forest>& reconstructMAF(bool orig);

    [[nodiscard]]
    bool checkMAF(const std::shared_ptr<graph::Forest>& mafSolution);

    [[nodiscard]]
    bool isSubtreeOfForest(const graph::Node* subtreeRoot, const std::shared_ptr<graph::Forest>& mafSolution, 
                           const std::shared_ptr<graph::Forest>& forest,
                           int subtreeID,
                           int bitmaskSize,
                           std::unordered_map<const graph::Node*, std::vector<uint64_t>>& nodeToSubtrees);
    

    public:
        /// \brief Constructor.
        /// \param instance The instance to solve.
        /// \param solverType The ILP solver to use.
        IncrementalMAFSolver(const std::shared_ptr<graph::Instance>& instance,
                    MaxSATSolverType solverType = MaxSATSolverType::UWrMaxSat);

        /// \brief Solves the instance.
        /// \returns true if solve was successful, otherwise false.
        bool solve() override;
};

}  // namespace solver

#endif  // PACE2026_MAFILPSOLVER_HPP