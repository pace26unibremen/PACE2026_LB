#include "IncrUWrMaxSATSolver.hpp"

#include "../../../../lib/UWrMaxSat/uwrmaxsat/ipamir.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <limits>

namespace solver {

IncrUWrMaxSATSolver::IncrUWrMaxSATSolver(std::chrono::steady_clock::time_point future) 
        :  future(future)
{}
IncrUWrMaxSATSolver::~IncrUWrMaxSATSolver()
{
    if (solver_ipamir) ipamir_release(solver_ipamir);
}


int terminationFunction(void * time)
{

    std::chrono::steady_clock::time_point* deadline = static_cast<std::chrono::steady_clock::time_point*>(time);

    if (*deadline < std::chrono::steady_clock::now())
    {
        return 1;
    }

    return 0;
}

void IncrUWrMaxSATSolver::stampSolver()
{
    if (solver_ipamir) ipamir_set_terminate(solver_ipamir, &future, terminationFunction);
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
        // at-least-one: alle Literale in einer Klausel
        for (int var : varIndices)
            ipamir_add_hard(solver_ipamir, var + 1);
        ipamir_add_hard(solver_ipamir, 0);  // Klausel-Ende
    }
    else
    {
        // force-false: jedes Literal einzeln als Unit-Klausel
        for (int var : varIndices)
            ipamir_add_hard(solver_ipamir, -(var + 1));
        ipamir_add_hard(solver_ipamir, 0);
    }
}

void IncrUWrMaxSATSolver::addAssumptions(const std::vector<double>& solValues)
{
    std::cout << "#r [DEBUG] addAssumptions() called\n";
    for (int i = 0; i < solValues.size(); ++i) {
        if (solValues[i] > 0.5) {
            ipamir_assume(solver_ipamir, i+1);
        } else {
            ipamir_assume(solver_ipamir, -(i+1));
        }
    }
}

ILPSolution IncrUWrMaxSATSolver::solve(int numVars)
{
    ILPSolution sol;

    // Output unterdrücken...
    auto start = std::chrono::high_resolution_clock::now();
    //std::streambuf* oldBuf = std::cout.rdbuf(nullptr);
    int result = ipamir_solve(solver_ipamir);
    //std::cout.rdbuf(oldBuf);
    auto end = std::chrono::high_resolution_clock::now();
    double timeSec = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();
    std::cout << "#r solve_time: " << timeSec << "s" << std::endl;


    if (result == 20 || result == 0)
        return sol; // UNSAT oder kein Ergebnis

    sol.feasible = true;
    sol.objValue = static_cast<double>(ipamir_val_obj(solver_ipamir));
    std::cout << "#r solve_score: " << sol.objValue + 1 << std::endl;
    sol.solValues.resize(numVars, 0.0);

    for (int i = 0; i < numVars; ++i)
    {
        int val = ipamir_val_lit(solver_ipamir, i+1);
        sol.solValues[i] = (val > 0) ? 1.0 : 0.0;
    }


    return sol;
}

} // namespace solver