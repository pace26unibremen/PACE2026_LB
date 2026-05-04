#include "UWrMaxSatSolver.hpp"
#include "ipamir.h"

#include <cassert>
#include <limits>
#include <iostream>

namespace solver {

ILPSolution UWrMaxSatSolver::solve(const ILPProblem problem)
{
    // Make sure it is a minimization problem...
    assert(problem.fMinimize);
    
    ILPSolution sol;

    // Setup...
    void* solver = ipamir_init();
    std::vector<int> varIDs = setup(solver, problem);

    // Suppress EvalMaxSAT output (o X lines)
    std::streambuf* oldBuf = std::cout.rdbuf(nullptr);

    // Start solve...
    int result = ipamir_solve(solver);

    // Restart output stream...
    std::cout.rdbuf(oldBuf);

    // Retrieve solution: best_goalvalue == Int_MAX means UNSAT...
    if (result == 20 || result == 0)
    {
        ipamir_release(solver);
        return sol;
    }

    sol.feasible = true;
    sol.objValue = static_cast<double>(ipamir_val_obj(solver));
    sol.solValues.resize(problem.nVars(), 0.0);

    for (int i = 0; i < problem.nVars(); ++i)
    {
        int val = ipamir_val_lit(solver, varIDs[i]);
        sol.solValues[i] = (val > 0) ? 1.0 : 0.0;
    }

    ipamir_release(solver);
    return sol;
}

std::vector<int> UWrMaxSatSolver::setup(void* solver, const ILPProblem& problem) const
{
    std::vector<int> varIDs(problem.nVars(), 0);

    // Add Variables...
    for (const ILPVariable& var : problem.vars)
    {
        // Make sure it is a binary variable...
        assert(var.isBinary);
        // Make sure objective coefficients are correct for MaxSAT...
        assert(var.objCoeff == 1.0);

        // IPAMIR Variablen sind 1-indiziert!
        int id = var.varNum + 1;
        varIDs[var.varNum] = id;

        // Soft clause: Variable soll false sein wenn möglich
        if (var.objCoeff > 0)
        {
            uint64_t weight = static_cast<uint64_t>(var.objCoeff + 0.5);
            ipamir_add_soft_lit(solver, id, weight);  // positives Literal = true kostet weight
        }
    }

    // Add Constraints...
    for (const ILPConstraint& constraint : problem.constraints)
    {
        // Make sure constraint is referencing variables...
        assert(!constraint.varIndices.empty());

        if (constraint.isLowerBound)
        {
            // sum(x_i) >= 1 → hard clause: mindestens eine Variable true
            for (int pos : constraint.varIndices)
                ipamir_add_hard(solver, varIDs[pos]);
            ipamir_add_hard(solver, 0);  // Clause abschließen
        }
        else
        {
            // Root constraint: Variable muss false sein
            for (int pos : constraint.varIndices)
            {
                ipamir_add_hard(solver, -varIDs[pos]);  // negiert = muss false sein
                ipamir_add_hard(solver, 0);
            }
        }
    }

    return varIDs;
}

} // namespace solver