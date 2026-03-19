#ifndef PACE2026_ABSTRACTILPSOLVER_HPP
#define PACE2026_ABSTRACTILPSOLVER_HPP

#include "ILPModel.hpp"

namespace solver {

/// \brief Abstract interface for ILP solver backends.
/// All concrete solvers (SCIP, CPLEX, EvalMaxSAT, UzL) must implement this interface.
class AbstractILPSolver
{
public:
    virtual ~AbstractILPSolver() = default;

    /// \brief Solves the ILP problem.
    /// \returns The ILP solution.
    [[nodiscard]]
    virtual ILPSolution solve(const ILPProblem problem) = 0;
};

}  // namespace solver

#endif  // PACE2026_ABSTRACTILPSOLVER_HPP