#include "EvalMaxSATSolver.hpp"

#include <cassert>

namespace solver {

ILPSolution EvalMaxSATSolver::solve(const ILPProblem problem)
{   
    // Make sure it is a minimization problem...
    assert(problem.fMinimize);

    // Create solver instance...
    EvalMaxSAT<> solver;
    ILPSolution sol;

    // Setup...
    std::vector<int> varIDs = setup(solver, problem);

    // Suppress EvalMaxSAT output (o X lines)
    std::streambuf* oldBuf = std::cout.rdbuf(nullptr);

    std::clog << "Starting Solve..."  << std::endl;
    // Start solve...
    bool sat = solver.solve();

    // Restart output stream...
    std::cout.rdbuf(oldBuf);

    // Retrieve solution...
    if (!sat) 
    {
        return sol; // by default solution is not feasible... 
    } 

    sol.feasible = true;
    sol.objValue = static_cast<double>(solver.getCost());
    sol.solValues.resize(problem.nVars(), 0.0);
    for(int i = 0; i < problem.nVars(); ++i)
        sol.solValues[i] = solver.getValue(varIDs[i]) ? 1.0 : 0.0;

    return sol;
}

std::vector<int> EvalMaxSATSolver::setup(EvalMaxSAT<>& solver, const ILPProblem& problem) const
{
    std::vector<int> varIDs(problem.nVars(), -1);
    
    // Add Variables...
    for (const ILPVariable& var : problem.vars)
    {   
        // Make sure it is a binary variable...
        assert(var.isBinary);
        // Make sure objecitve coefficients are correkt for MaxSAT...
        assert(var.objCoeff == 1.0);

        varIDs[var.varNum] = solver.newVar();
        // Add Soft clause for each variable, meaning that it should not be cut, if not necessary
        if(var.objCoeff > 0)
        {
            int weight = static_cast<int>(var.objCoeff + 0.5);
            solver.addClause({-varIDs[var.varNum]}, weight);
        }
    }

    // Add Constraints...
    for (int i = 0; i < problem.nConstraints(); ++i)
    {
        int start = problem.constraintStart[i];
        int end   = (i + 1 < problem.nConstraints())
                    ? problem.constraintStart[i + 1]
                    : (int)problem.allVarIndices.size();

        if (problem.isLowerBound[i])
        {
            std::vector<int> clause;
            for (int j = start; j < end; ++j)
                clause.push_back(varIDs[problem.allVarIndices[j]]);
            solver.addClause(clause);
        }
        else // root constraint
        {
            for (int j = start; j < end; ++j)
                solver.addClause({-varIDs[problem.allVarIndices[j]]});
        }
    }

    return varIDs;
}
} // namespace solver