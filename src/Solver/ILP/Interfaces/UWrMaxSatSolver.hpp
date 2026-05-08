#ifndef PACE2026_UWRMAXSATSOLVER_HPP
#define PACE2026_UWRMAXSATSOLVER_HPP

#include "../AbstractILPSolver.hpp"
#include "../ILPModel.hpp"
#ifdef USE_MAXPRE
#include "preprocessorinterface.hpp"
#endif

#include <vector>

namespace solver {

/// \brief Concrete ILP solver backend using UWrMaxSat.
/// Translates an ILPProblem into UWrMaxSat (via MsSolver).
///
/// \note The ILPProblem is expected to be a Hitting Set formulation:
///       - All variables are binary with objective coefficient 1.0
///       - All constraints are of the form: sum(x_i) >= 1
///       - The root variable constraint is: x_root <= 0 (upper bound)
class UWrMaxSatSolver : public AbstractILPSolver
{
public:
    UWrMaxSatSolver() = default;
    ~UWrMaxSatSolver() override = default;

    /// \brief Solves the ILP problem using UWrMaxSat Solver.
    /// \note Creates MsSolver instance, starts Solver and writes solution.
    /// \param problem The ILP problem to solve.
    /// \returns The ILP solution. solution.feasible is false if no solution was found.
    [[nodiscard]]
    ILPSolution solve(const ILPProblem problem) override;

private:
    /// \brief Sets up all variables and constraints in the UWrMaxSat solver.
    /// Variables are added as soft clauses (minimize number of cut edges).
    /// Constraints are added as hard clauses (hitting set + root constraint).
    /// \param solver The solver instance to populate.
    /// \param problem The ILP problem to translate.
    /// \return Vector of Integers (Variable IDs) of IPAMIR Instance.
    [[nodiscard]]
    std::vector<int> setup(void* solver, const ILPProblem& problem) const;

#ifdef USE_MAXPRE
private:
    /// \brief Helping Struct that stores all necessary data for MaxPre Preprocessing...
    struct MaxPreData {

        /// \brief Vector of clauses as input for Preprocessing...
        std::vector<std::vector<int>> clauses;

        /// \brief Weight of each constraint.
        std::vector<uint64_t> weights;

        /// \brief Top weight for all hard clauses (to ensure these are fulfilled).
        const uint64_t topWeight = UINT64_MAX;

        /// \brief Preprocessed Constraits given back from Preprocessor...
        std::vector<std::vector<int>> ppClauses;

        /// \brief Weights of preprocessed constraits given back from Preprocessor...
        std::vector<uint64_t> ppWeights;

        /// \brief Labels of new problem formulation after preprocessing (needed for reconstruction).
        std::vector<int> ppLabels;

        /// \brief Varibale IDs of Preprocessed Variables (needed for reconstruction).
        std::vector<int> ppVarIDs;
    };

    /// \brief Transforms all Varbales & Constraints into Weighted Partial MaxSat Format for MaxPre Preprocessor.
    /// \param problem Representation of ILPProblem (incl. all Variables and Constraints).
    /// \return Instance of MaxPreData with encoding of MaxSat Problem.
    [[nodiscard]]
    MaxPreData buildMaxPre(const ILPProblem& problem) const;

    /// \brief Sets up all preprocessed clausels in the UWrMaxSat solver.
    /// \param solver The solver instance to populate.
    /// \param data The Preprocessed Problem to solve.
    void setupPreprocessedProblem(void* solver, MaxPreData& data) const;

    /// 
    ILPSolution reconstructMaxPre(void* solver, const MaxPreData& data, 
                                   maxPreprocessor::PreprocessorInterface& maxpre,
                                   const ILPProblem& problem) const;
#endif

};

} // namespace solver

#endif // PACE2026_UWRMAXSATSOLVER_HPP