#ifndef PACE2026_INCRUWRMAXSATSOLVER_HPP
#define PACE2026_INCRUWRMAXSATSOLVER_HPP

#include "ILPModel.hpp"
#include "chrono"

namespace solver {

class IncrUWrMaxSATSolver
{
public:
    ~IncrUWrMaxSATSolver();

    /// \brief Initialises the solver.
    void initSolver(int numVars);

    /// \brief Adds a soft clause (minimize number of falsified soft clauses).
    /// \param id Id of variable (positive = var, negative = negated var).
    /// \param objCoeff Weight of the soft clause.
    void addSoftClause(int id, double objCoeff);

    /// \brief Adds a hard clause (must be satisfied).
    /// \param varIndices Literals (positive = var, negative = negated var).
    /// \param isLowerBound Flag to decide, which type of clause.
    void addHardClause(const std::vector<int>& varIndices, bool isLowerBound);

    /// \brief Solves the current problem.
    /// \param numVars Number of Variables (needed for Solution).
    /// \returns true if satisfiable, false otherwise.
    ILPSolution solve(int numVars);


private:
    void* solver_ipamir = nullptr;

}; // class IncrUWrMaxSatSolver

} // namespace solver

#endif // PACE2026_INCRUWRMAXSATSOLVER_HPP