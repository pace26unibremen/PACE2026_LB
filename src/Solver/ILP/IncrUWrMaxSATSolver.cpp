#include "IncrUWrMaxSATSolver.hpp"

#include "../../../../lib/UWrMaxSat/uwrmaxsat/ipamir.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <limits>

namespace solver {

IncrUWrMaxSATSolver::~IncrUWrMaxSATSolver()
{
    if (solver_ipamir) ipamir_release(solver_ipamir);
}

void IncrUWrMaxSATSolver::initSolver(int numVars)
{
    if (solver_ipamir) ipamir_release(solver_ipamir);
    solver_ipamir = ipamir_init();
}

void IncrUWrMaxSATSolver::addSoftClause(int id, double objCoeff)
{
    uint64_t weight = static_cast<uint64_t>(objCoeff + 0.5);
    ipamir_add_soft_lit(solver_ipamir, id + 1, weight);
}

void IncrUWrMaxSATSolver::addHardClause(const std::vector<int>& varIndices, bool isLowerBound)
{
    if (isLowerBound)
    {
        for (int var : varIndices)
            ipamir_add_hard(solver_ipamir, var + 1);
        ipamir_add_hard(solver_ipamir, 0);
    }
    else
    {
        for (int var : varIndices)
            ipamir_add_hard(solver_ipamir, -(var + 1));
        ipamir_add_hard(solver_ipamir, 0);
    }
}

ILPSolution IncrUWrMaxSATSolver::solve(int numVars)
{
    ILPSolution sol;

    std::streambuf* oldBuf = std::cout.rdbuf(nullptr);
    int result = ipamir_solve(solver_ipamir);
    std::cout.rdbuf(oldBuf);
    auto end = std::chrono::high_resolution_clock::now();


    if (result == 20 || result == 0)
        return sol;

    sol.feasible = true;
    sol.objValue = static_cast<double>(ipamir_val_obj(solver_ipamir));
    sol.solValues.resize(numVars, 0.0);

    for (int i = 0; i < numVars; ++i)
    {
        int val = ipamir_val_lit(solver_ipamir, i+1);
        sol.solValues[i] = (val > 0) ? 1.0 : 0.0;
    }


    return sol;
}

} // namespace solver