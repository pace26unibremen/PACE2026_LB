#ifndef PACE2026_EVALMAXSATSOLVER_HPP
#define PACE2026_EVALMAXSATSOLVER_HPP

#include "../AbstractILPSolver.hpp"
#include "../ILPModel.hpp"
#include "EvalMaxSAT.h"

#include <vector>

namespace solver {

/// \brief Concrete ILP solver backend using EvalMaxSAT.
/// Translates an ILPProblem into EvalMaxSAT
///
/// \note The ILPProblem is expected to be a Hitting Set formulation:
///       - All variables are binary with objective coefficient 1.0
///       - All constraints are of the form: sum(x_i) >= 1
///       - The root variable constraint is: x_root <= 0 (upper bound)
class EvalMaxSATSolver : public AbstractILPSolver
{
public:
    EvalMaxSATSolver() = default;
    ~EvalMaxSATSolver() override = default;

    /// \brief Solves the ILP problem using EvalMaxSAT Solver.
    /// \note Creates EvalMAXSAT instance, starts Solver and writes solution.
    /// \param problem The ILP problem to solve.
    /// \returns The ILP solution. solution.feasible is false if no solution was found.
    [[nodiscard]]
    ILPSolution solve(const ILPProblem problem) override;

private:
    /// \brief Sets up all variables and constraints in the EvalMaxSAT solver.
    /// Variables are added as soft clauses (minimize number of cut edges).
    /// Constraints are added as hard clauses (hitting set + root constraint).
    /// \param evalSolver The EvalMaxSAT solver instance to populate.
    /// \param problem The ILP problem to translate.
    /// \return Vector of variable indizes of EvalMaxSat Instance.
    [[nodiscard]]
    std::vector<int> setup(EvalMaxSAT<>& evalSolver, const ILPProblem& problem) const;

}; // class EvalMaxSAT


} // namespace solver

#endif // PACE2026_EVALMAXSATSOLVER_HPP