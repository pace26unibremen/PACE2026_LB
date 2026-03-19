#ifndef PACE2026_MAFILPSOLVER_HPP
#define PACE2026_MAFILPSOLVER_HPP

#include "AbstractILPSolver.hpp"
#include "../../Graph/Forest.hpp"
#include "../../Graph/Instance.hpp"
#include "../AbstractSolver.hpp"
#include "ILPFormulation.hpp"

#include <vector>
#include <memory>

namespace solver {

/// \brief Available ILP solver.
enum class ILPSolverType {
    SCIP,
    CPLEX,
    EvalMaxSAT,
    HittingSet
};

/// \brief Solves the MAF problem exactly via ILP formulation.
/// Orchestrates ILPFormulation and a concrete ILP solver.
/// \note Tree reductions should be applied to the instance BEFORE
///       passing it to this solver. This solver assumes the instance
///       is already reduced.
/// \note The Forest must not be modified after build() is called,
///       as internal node pointers would become invalid.
class MAFILPSolver : public AbstractSolver
{
    private:
    /// \brief The ILP solver.
    std::unique_ptr<AbstractILPSolver> ilpSolver;

    /// \brief Builds the ILP problem from the instance.
    /// \returns The ILP problem ready to be passed to the solver.
    [[nodiscard]]
    ILPProblem buildProblem() const;

    /// \brief Solves the ILP problem.
    /// \param problem The ILP problem to solve.
    /// \returns The ILP solution.
    [[nodiscard]]
    ILPSolution solveProblem(const ILPProblem problem) const;

    /// \brief Extracts cut edges from a solution.
    /// \param solution The solution returned by the solver.
    /// \returns Vector of cut edge indices (0-indexed).
    [[nodiscard]]
    std::vector<int> extractCutEdges(const ILPSolution& solution) const;

    /// \brief Reconstructs the MAF from cut edges.
    /// \param cutEdges The cut edges.
    /// \returns The MAF as a Forest.
    /// \todo Implement MAF reconstruction from cut edges.
    [[nodiscard]]
    std::shared_ptr<graph::Forest> reconstructMAF(const std::vector<int>& cutEdges) const;

    /// \brief Creates the concrete ILP solver based on solverType.
    /// \param solverType The solver type to create.
    /// \returns A unique pointer to the concrete solver.
    static std::unique_ptr<AbstractILPSolver> createSolver(ILPSolverType solverType);

    public:
        /// \brief Constructor.
        /// \param instance The instance to solve.
        /// \param solverType The ILP solver to use.
        MAFILPSolver(const std::shared_ptr<graph::Instance>& instance,
                    ILPSolverType solverType = ILPSolverType::EvalMaxSAT);

        /// \brief Solves the instance.
        /// \returns The MAF as a Forest.
        std::shared_ptr<graph::Forest> solve() override;
};

}  // namespace solver

#endif  // PACE2026_MAFILPSOLVER_HPP