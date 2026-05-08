#ifdef USE_MAXPRE
#include "preprocessorinterface.hpp"
#endif

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

// If MaxPre Solver is activated, convert problem to MaxPre format...
#ifdef USE_MAXPRE
    std::vector<std::vector<int>> clauses;
    std::vector<uint64_t> weights;
    uint64_t topWeight = UINT64_MAX;  // weight of hard clauses

    // New Index for MaxPre
    std::vector<int> varIDs(problem.nVars());
    for (int i = 0; i < problem.nVars(); i++) varIDs[i] = i + 1;

    // Soft clauses (Variables)
    for (const ILPVariable& var : problem.vars) {
        if (var.objCoeff > 0) {
            clauses.push_back({-varIDs[var.varNum]});
            weights.push_back(static_cast<uint64_t>(var.objCoeff + 0.5));
        }
    }

    // Hard clauses (Constraints)
    for (const ILPConstraint& constraint : problem.constraints) {
        std::vector<int> clause;
        if (constraint.isLowerBound) {
            for (int pos : constraint.varIndices)
                clause.push_back(varIDs[pos]);
        } else {
            for (int pos : constraint.varIndices)
                clause.push_back(-varIDs[pos]);
        }
        clauses.push_back(clause);
        weights.push_back(topWeight);
    }

    // MaxPre preprocessing
    maxPreprocessor::PreprocessorInterface maxpre(clauses, weights, topWeight);
    maxpre.preprocess("[uvsrgc]", 0);  // 0 = kein Log-Output

    // Get simplified Problem from Preprocessor
    std::vector<std::vector<int>> ppClauses;
    std::vector<uint64_t> ppWeights;
    std::vector<int> ppLabels;
    maxpre.getInstance(ppClauses, ppWeights, ppLabels);

    // Solve simplified problem...
    void* solver = ipamir_init();

    for (int i = 0; i < ppClauses.size(); i++) {
        if (ppWeights[i] == topWeight) {
            // Hard clause
            for (int lit : ppClauses[i]) ipamir_add_hard(solver, lit);
            ipamir_add_hard(solver, 0);
        } else {
            // Soft clause
            assert(ppClauses[i].size() == 1);
            int softLit = ppClauses[i][0];
            ipamir_add_soft_lit(solver, -softLit, ppWeights[i]);
        }
    }

    int result = ipamir_solve(solver);

    // Reconstruct Solution...
    std::vector<int> trueLiterals;
    int maxVar = 0;
    for (auto& c : ppClauses)
        for (int lit : c) maxVar = std::max(maxVar, std::abs(lit));

    for (int v = 1; v <= maxVar; v++) {
        int val = ipamir_val_lit(solver, v);
        trueLiterals.push_back(val > 0 ? v : -v);
    }

    std::vector<int> model = maxpre.reconstruct(trueLiterals);
    
    sol.feasible = true;
    sol.objValue = static_cast<double>(ipamir_val_obj(solver));
    sol.solValues.resize(problem.nVars(), 0.0);
    for (int i = 0; i < problem.nVars(); i++) {  
        sol.solValues[i] = (model[i] > 0) ? 1.0 : 0.0;
    }

    ipamir_release(solver);

#else
    // Setup...
    void* solver = ipamir_init();
    std::vector<int> varIDs = setup(solver, problem);

    // Suppress Solver output (o X lines)
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
#endif

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

        // IPAMIR Variablen are 1-indexed!
        int id = var.varNum + 1;
        varIDs[var.varNum] = id;

        if (var.objCoeff > 0)
        {
            uint64_t weight = static_cast<uint64_t>(var.objCoeff + 0.5);
            ipamir_add_soft_lit(solver, id, weight);  
        }
    }

    // Add Constraints...
    for (const ILPConstraint& constraint : problem.constraints)
    {
        // Make sure constraint is referencing variables...
        assert(!constraint.varIndices.empty());

        if (constraint.isLowerBound)
        {
            
            for (int pos : constraint.varIndices)
                ipamir_add_hard(solver, varIDs[pos]);
            ipamir_add_hard(solver, 0);  // finish clause
        }
        else
        {
            // Root constraint: Variable muss false sein
            for (int pos : constraint.varIndices)
            {
                ipamir_add_hard(solver, -varIDs[pos]);  
                ipamir_add_hard(solver, 0);
            }
        }
    }

    return varIDs;
}

} // namespace solver