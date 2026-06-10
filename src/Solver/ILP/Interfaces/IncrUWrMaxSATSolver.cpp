#include "IncrUWrMaxSATSolver.hpp"
#include "ipamir.h"

#include <cassert>
#include <limits>
#include <iostream>

namespace solver {

IncrUWrMaxSatSolver::~IncrUWrMaxSatSolver()
{
    if (solver_ipamir) ipamir_release(solver_ipamir);
}

void IncrUWrMaxSatSolver::initSolver()
{
    if (solver_ipamir) ipamir_release(solver_ipamir);
    solver_ipamir = ipamir_init();
}

void IncrUWrMaxSatSolver::addSoftLit(int id, double objCoeff)
{
    uint64_t weight = static_cast<uint64_t>(objCoeff + 0.5);
    ipamir_add_soft_lit(solver_ipamir, id + 1, weight);
}

void IncrUWrMaxSatSolver::addHardClause(const std::vector<int>& varIDs, bool isLowerBound)
{
    if (isLowerBound)
    {
        // at-least-one: alle Literale in einer Klausel
        for (int var : varIDs)
            ipamir_add_hard(solver_ipamir, var + 1);
        ipamir_add_hard(solver_ipamir, 0);  // Klausel-Ende
    }
    else
    {
        // force-false: jedes Literal einzeln als Unit-Klausel
        for (int var : varIDs)
        {
            ipamir_add_hard(solver_ipamir, -(var + 1));
            ipamir_add_hard(solver_ipamir, 0);
        }
    }
}

ILPSolution IncrUWrMaxSatSolver::solve(int numVars)
{
    ILPSolution sol;

    // Output unterdrücken...
    std::streambuf* oldBuf = std::cout.rdbuf(nullptr);
    int result = ipamir_solve(solver_ipamir);
    std::cout.rdbuf(oldBuf);

    if (result == 20 || result == 0)
        return sol; // UNSAT oder kein Ergebnis

    sol.feasible = true;
    sol.objValue = static_cast<double>(ipamir_val_obj(solver_ipamir));
    sol.solValues.resize(numVars, 0.0);

    for (int i = 0; i < numVars; ++i)
    {
        int val = ipamir_val_lit(solver, i+1);
        sol.solValues[i] = (val > 0) ? 1.0 : 0.0;
    }

    return sol;
}

} // namespace solver