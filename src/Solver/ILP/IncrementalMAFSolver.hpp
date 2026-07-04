#ifndef PACE2026_INCREMENTALILPSOLVER_HPP
#define PACE2026_INCREMENTALILPSOLVER_HPP

#include "../../Graph/Forest.hpp"
#include "../../Graph/Instance.hpp"
#include "../AbstractSolver.hpp"
#include "../Action/DeleteEdgeAction.hpp"
#include "../Cluster/LeastCommonAncestor.hpp"
#include "../Context.hpp"
#include "AbstractIncrementalSolver.hpp"
#include "ILPModel.hpp"
#include "Interfaces/IncrUWrMaxSATSolver.hpp"

#include <array>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace solver {

/// \brief Available MaxSAT solver.
enum class MaxSATSolverType {
    UWrMaxSAT,
    EvalMaxSAT
};

enum class ConstraintCheckType {
    Linear,
    Coverage,
    All,
};

class IncrementalMAFSolver : public AbstractSolver
{
    /// \brief Represents a LeafPair of a Solution for the MAF-Problem.
    /// \note this is used for computing the pathpair constraints across subtrees in the MAF.
    struct LeafPair {
        /// \brief Represents the left (first) leaf of the pair.
        unsigned int l;
        /// \brief Represents the right (second) leaf of the pair.
        unsigned int r;
    };
    
    static constexpr std::array<ConstraintCheckType,3> ALL_CHECK_TYPES= {
        ConstraintCheckType::Linear,
        ConstraintCheckType::Coverage,
        ConstraintCheckType::All
    };

    private:
        /// \brief Context information about the instance and the solver state
        std::shared_ptr<Context> context = std::make_shared<Context>();

        /// \brief Vector of Constraints for reinitialisation of UWrMaxSAT Solver
        std::vector<std::vector<int>> constraints;

        /// \brief Counter for total number of constraints...
        int cnt_constraints;

        /// \brief The Incremental MaxSAT solver.
        std::unique_ptr<IncrUWrMaxSATSolver> solver;

        /// \brief Data of First Tree (Forest) of binary MAF-Problem.
        ForestData forestData_1;
        /// \brief Data of Second Tree (Forest) of binary MAF-Problem.
        ForestData forestData_2;

        /// \brief Data of Second Tree (Forest) of binary MAF-Problem.
        double timeOutDelay = 30;

        /// \brief Set for checking if triple-constraints are already added to solver.
        std::unordered_set<std::string> addedTripleConstraints;
        /// \brief Set for checking if pathpair-constraints are already added to solver.
        std::unordered_set<std::string> addedPathPairConstraints;

        /// \brief Max Number of Constraints added to Solver in each round.
        int MAX_CONSTRAINTS_PER_ROUND;
        int MIN_CONSTRAINTS_PER_ROUND;
        int MAX_CONSTRAINTS_PER_ROUND_PER_SUBTREE;

        int currentLB;

        /// \brief stores all applied rules of the current branch in the order in which they were applied.
        ///std::list<std::shared_ptr<DeleteEdgeAction>> appliedActions = std::list<std::shared_ptr<DeleteEdgeAction>>();

        /// \brief Creates the concrete ILP solver based on solverType.
        /// \param solverType The solver type to create.
        void buildSolver(MaxSATSolverType solverType);

        /// \brief Checks wether the current solution is a correct MAF.
        /// \param mafSolution Current Solution.
        /// \return true, if MAF is correct, false otherwise.
        /// \note If current solution is not a correct MAF, the function generates violated constraints and adds it to the solver.
        ///       For this there can be different strategies in use.
        [[nodiscard]]
        bool checkMAF(std::shared_ptr<graph::Forest>& mafSolution);

        /// \brief Adds initial constraints to solver.
        /// \note In the Basic config there is only a linear passing of all leafs for triple and pathpair constraints...
        void addInitialConstraints();

        /// \brief Generates the triple constraint, based on the given three leaves...
        /// \param label1 First Leaf.
        /// \param label2 Second Leaf.
        /// \param label3 Third Leaf.
        void generateTripleConstraint(unsigned int label1, unsigned int label2, unsigned int label3);

        // \brief Generates the pathpair constraint, based on the given pairs...
        /// \param lpair1 Left leaf of first pair.
        /// \param lpair2 Left leaf of second pair.
        /// \param rpair1 Right leaf of first pair.
        /// \param rpair2 Right leaf of first pair.
        void generatePathPairConstraint(unsigned int lpair1, unsigned int lpair2, 
                                        unsigned int rpair1, unsigned int rpair2);

        /// \brief Checks if there are still triple constraints not satisifed by current (subtree) solution.
        /// \param n_constraints Current number of constraints in round.
        /// \param labels Leafs of current subtree.
        /// \param type Decides wether to have an incomplete linear pass or full pass.
        /// \param forestData_sol Data Object of current solution.
        /// \return Updated number of constraints in round
        /// \note If there are unsatisfied triple constraints, these will be directly added to the solver...
        [[nodiscard]]
        int checkTripleConstraints(int n_constraints, std::vector<unsigned int> labels, ConstraintCheckType type,
                                    const ForestData& forestData_sol);
        
        /// \brief Checks if there are still pathpair constraints not satisifed by current (subtree) solution.
        /// \param n_constraints Current number of constraints in round.
        /// \param type Decides wether to have an incomplete linear pass or full pass.
        /// \param forestData_sol Data Object of current solution.
        /// \return Updated number of constraints in round
        /// \note If there are unsatisfied triple constraints, these will be directly added to the solver...
        [[nodiscard]]                         
        int checkPathPairConstraints(int n_constraints, ConstraintCheckType type, 
                                     const std::vector<IncrementalMAFSolver::LeafPair>& pairs,
                                     const ForestData& forestData_sol);
        

        /// \brief Extracts cut edges from a solution.
        /// \param solution The solution returned by the solver.
        /// \returns Vector of cut edge indices (0-indexed).
        [[nodiscard]]
        std::vector<int> extractCutEdges(const ILPSolution& ilpSolution);

        /// \brief Reconstructs the MAF from cut edges.
        /// \param cutEdges The cut edges.
        /// \param orig Decides wether Solution can be applied to original instance, instead of copy.
        /// \return A pointer to the new MAF Solution.
        std::shared_ptr<graph::Forest> reconstructMAF(std::vector<int> cutEdges, bool orig);
        
        /// \brief Reconstructs Forest_1, if solution is not a true MAF.
        /// void restoreForest();

        /// \brief Generates all leaf pairs across subtrees of current solution.
        /// \param mafSolution Current solution.
        /// \return A set of LeafPairs for current solution.
        [[nodiscard]]
        std::vector<LeafPair> generateLeafPairs(const std::shared_ptr<graph::Forest>& mafSolution);

        /// \brief Generates all leaf pairs for a subtree of current solution.
        /// \param rootNode Current solution.
        /// \return A set of LeafPairs for current solution.
        [[nodiscard]]
        std::vector<LeafPair> generateLeafPairs(const graph::Node* rootNode, const std::shared_ptr<graph::Forest>& mafSolution);
        
        /// \brief Calculates the leafs of a Subtree of the MAF, given the root Node of the subtree.
        /// \param subtreeRoot Root Node of subtree.
        /// \return Set of labels of the leafs in the subtree.
        [[nodiscard]]
        std::vector<unsigned int> getSubtreeLabels(const graph::Node* subtreeRoot, const std::shared_ptr<graph::Forest>& forest);

    public:
        /// \brief Constructor.
        /// \param instance The instance to solve.
        /// \param solverType The ILP solver to use.
        IncrementalMAFSolver(const std::shared_ptr<graph::Instance>& instance);

        /// \brief Constructor.
        /// \param instance The instance to solve.
        /// \param solverType The ILP solver to use.
        /// \param context additional context for the instance
        IncrementalMAFSolver(const std::shared_ptr<graph::Instance>& instance, const std::shared_ptr<solver::Context>& context);

        /// \brief Solves the instance.
        /// \returns true if solve was successful, otherwise false.
        bool solve() override;

        void setTimeOut(double time);

};

}  // namespace solver

#endif  // PACE2026_MAFILPSOLVER_HPP