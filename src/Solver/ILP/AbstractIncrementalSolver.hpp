#ifndef PACE2026_ABSTRACTINCREMENTALSOLVER_HPP
#define PACE2026_ABSTRACTINCREMENTALSOLVER_HPP

#include <vector>
#include "ILPModel.hpp"

namespace solver {

/// \brief Abstract interface for incremental MaxSAT solvers.
///        Supports adding clauses incrementally between solve() calls.
class AbstractIncrementalSolver
{
    public:
        virtual ~AbstractIncrementalSolver() = default;

        /// \brief Initialises the solver.
        virtual void initSolver() = 0;

        /// \brief Release solver.
        virtual void releaseSolver() = 0;

        /// \brief Adds a soft clause (minimize number of falsified soft clauses).
        /// \param id Id of variable (positive = var, negative = negated var).
        /// \param weight Weight of the soft clause.
        virtual void addSoftClause(int id, double objCoeff) = 0;

        /// \brief Adds a hard clause (must be satisfied).
        /// \param varIDs Literals (positive = var, negative = negated var).
        virtual void addHardClause(const std::vector<int>& varIDs, bool isLowerBound) = 0;

        /// \brief Solves the current problem.
        /// \param numVars Number of Variables (needed for Solution).
        /// \returns true if satisfiable, false otherwise.
        virtual ILPSolution solve(int numVars) = 0;

}; // class AbstractIncrementalSolver

} // namespace solver

#endif // PACE2026_ABSTRACTINCREMENTALSOLVER_HPP