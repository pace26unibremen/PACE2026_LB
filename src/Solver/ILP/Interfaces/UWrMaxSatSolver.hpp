#ifndef PACE2026_UWRMAXSATSOLVER_HPP
#define PACE2026_UWRMAXSATSOLVER_HPP

#include "../AbstractILPSolver.hpp"
#include "../ILPModel.hpp"

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
    /// \return Vector of Integers of IPAMIR Instance.
    [[nodiscard]]
    std::vector<int> setup(void* solver, const ILPProblem& problem) const;

};

} // namespace solver

#endif // PACE2026_UWRMAXSATSOLVER_HPP