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
    UWrMaxSAT
};



inline std::ostream& operator<<(std::ostream& os, MaxSATSolverType type)
{
    switch (type)
    {
        case MaxSATSolverType::UWrMaxSAT:  os << "UWrMaxSat";  break;
        default: os << "Unknown"; break;
    }
    return os;
}

class IncrementalMAFSolver : public AbstractSolver
{
    struct LeafPair {
        unsigned int l;
        unsigned int r;
    };

    static constexpr int MAX_ROUNDS = 10000;
    static constexpr int MAX_CONSTRAINTS_PER_ROUND = 10000;
    private:
    std::vector<std::vector<int>> constraints;
    std::unique_ptr<AbstractIncrementalSolver> solver;
    std::shared_ptr<graph::Forest> forest_1;
    std::shared_ptr<graph::Forest> forest_2;
    std::unordered_map<const graph::Node*, int> nodeToIndex1;
    std::unordered_map<const graph::Node*, int> nodeToIndex2;
    /// \brief Precomputed LCA table for forest1.
    /// \note Must be reinstantiated if forest1 changes.
    std::shared_ptr<cluster::LeastCommonAncestor> lca1;

    /// \brief Precomputed LCA table for forest2.
    /// \note Must be reinstantiated if forest2 changes.
    std::shared_ptr<cluster::LeastCommonAncestor> lca2;


    /// \brief Creates the concrete ILP solver based on solverType.
    /// \param solverType The solver type to create.
    void buildSolver(MaxSATSolverType solverType);

    /// \brief Adds initial constraints to solver.
    void addInitialConstraints();

    
    void generateTripleConstraint(unsigned int label1, unsigned int label2, unsigned int label3, bool initial);

    void generatePathPairConstraint(unsigned int lpair1, unsigned int lpair2, 
                                                          unsigned int rpair1, unsigned int rpair2);
    int checkTripleConstraints(int numLeaves, int n_constraints, std::vector<unsigned int> labels,
                                                std::shared_ptr<graph::Forest>mafSolution,
                                                std::shared_ptr<cluster::LeastCommonAncestor> lca_sol,
                                                std::unordered_map<const graph::Node*, int> nodeToIndex_sol);

    int checkPathPairConstraints(int n_constraints,
                                                std::shared_ptr<graph::Forest>mafSolution,
                                                std::shared_ptr<cluster::LeastCommonAncestor> lca_sol,
                                                std::unordered_map<const graph::Node*, int> nodeToIndex_sol);
    

    /// \brief Extracts cut edges from a solution.
    /// \param solution The solution returned by the solver.
    /// \returns Vector of cut edge indices (0-indexed).
    [[nodiscard]]
    std::vector<int> extractCutEdges(const ILPSolution& ilpSolution);

    /// \brief Reconstructs the MAF from cut edges.
    /// \param cutEdges The cut edges.
    /// \return 
    std::shared_ptr<graph::Forest> reconstructMAF(std::vector<int> cutEdges, bool orig);

    [[nodiscard]]
    bool checkMAF(std::shared_ptr<graph::Forest>& mafSolution);

    [[nodiscard]]
    std::vector<LeafPair> generateLeafPairs(std::shared_ptr<graph::Forest>& mafSolution);

    [[nodiscard]]
    std::vector<unsigned int> getSubtreeLabels(const graph::Node* subtreeRoot);

    public:
        /// \brief Constructor.
        /// \param instance The instance to solve.
        /// \param solverType The ILP solver to use.
        IncrementalMAFSolver(const std::shared_ptr<graph::Instance>& instance,
                    MaxSATSolverType solverType = MaxSATSolverType::UWrMaxSAT);

        /// \brief Solves the instance.
        /// \returns true if solve was successful, otherwise false.
        bool solve() override;
};

}  // namespace solver

#endif  // PACE2026_MAFILPSOLVER_HPP