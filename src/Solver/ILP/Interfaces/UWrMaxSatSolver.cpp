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

    void* solver = ipamir_init();
    ILPSolution sol;

// If MaxPre Solver is activated, convert problem to MaxPre format...
#ifdef USE_MAXPRE
    
    auto data = UWrMaxSatSolver::buildMaxPre(problem);

    // MaxPre preprocessing
    maxPreprocessor::PreprocessorInterface maxpre(data.clauses, data.weights, data.topWeight);
    maxpre.preprocess("[uvsrgc]", 0);  // 0 = no Log-Output

    // Get simplified Problem from Preprocessor
    std::vector<std::vector<int>> ppClauses;
    std::vector<uint64_t> ppWeights;
    std::vector<int> ppLabels;
    maxpre.getInstance(ppClauses, ppWeights, ppLabels);

    data.ppClauses = ppClauses;
    data.ppWeights = ppWeights;
    data.ppLabels = ppLabels;

    // Setup for IPAMIR Solver...
    UWrMaxSatSolver::setupPreprocessedProblem(solver, data);

    std::clog << "Starting Solve..."  << std::endl;
    int result = ipamir_solve(solver);

    if (result == 20 || result == 0) {
        ipamir_release(solver);
        return sol;
    }

    sol = UWrMaxSatSolver::reconstructMaxPre(solver, data, maxpre, problem);

#else
    std::vector<int> varIDs = setup(solver, problem);

    // Suppress Solver output (o X lines)
    std::streambuf* oldBuf = std::cout.rdbuf(nullptr);

    std::clog << "Starting Solve..."  << std::endl;
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

#endif

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
    for (int i = 0; i < problem.nConstraints(); ++i)
    {
        int start = problem.constraintStart[i];
        int end   = (i + 1 < problem.nConstraints()) 
                    ? problem.constraintStart[i + 1] 
                    : (int)problem.allVarIndices.size();

        if (problem.isLowerBound[i])
        {
            for (int j = start; j < end; ++j)
                ipamir_add_hard(solver, varIDs[problem.allVarIndices[j]]);
            ipamir_add_hard(solver, 0);
        }
        else
        {
            for (int j = start; j < end; ++j)
            {
                ipamir_add_hard(solver, -varIDs[problem.allVarIndices[j]]);
                ipamir_add_hard(solver, 0);
            }
        }
    }

    return varIDs;
}

#ifdef USE_MAXPRE

UWrMaxSatSolver::MaxPreData UWrMaxSatSolver::buildMaxPre(const ILPProblem& problem) const
{
    UWrMaxSatSolver::MaxPreData data;
    std::vector<std::vector<int>> clauses;
    std::vector<uint64_t> weights;

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
    for (int i = 0; i < problem.nConstraints(); ++i)
    {
        int start = problem.constraintStart[i];
        int end   = (i + 1 < problem.nConstraints()) 
                    ? problem.constraintStart[i + 1] 
                    : (int)problem.allVarIndices.size();

        std::vector<int> clause;
        if (problem.isLowerBound[i]) {
            for (int j = start; j < end; ++j)
                clause.push_back(varIDs[problem.allVarIndices[j]]);
        } else {
            for (int j = start; j < end; ++j)
                clause.push_back(-varIDs[problem.allVarIndices[j]]);
        }
        clauses.push_back(clause);
        weights.push_back(data.topWeight);
    }

    data.clauses = clauses;
    data.weights = weights;

    return data;
}

void UWrMaxSatSolver::setupPreprocessedProblem(void* solver, UWrMaxSatSolver::MaxPreData& data) const
{
    std::vector<int> ppVarIDs;

    // Hard clauses
    for (int i = 0; i < (int)data.ppClauses.size(); i++) {
        if (data.ppWeights[i] == data.topWeight) {
            for (int lit : data.ppClauses[i]) ipamir_add_hard(solver, lit);
            ipamir_add_hard(solver, 0);
        } else {
            assert(data.ppClauses[i].size() == 1);
            ipamir_add_soft_lit(solver, -data.ppClauses[i][0], data.ppWeights[i]);
        }
    }

    // ppLabels sind die Variablen-IDs - analog zu varIDs
    for (int label : data.ppLabels)
        ppVarIDs.push_back(std::abs(label));

    data.ppVarIDs = ppVarIDs;
}

ILPSolution UWrMaxSatSolver::reconstructMaxPre(void* solver, const UWrMaxSatSolver::MaxPreData& data, 
                                    maxPreprocessor::PreprocessorInterface& maxpre,
                                    const ILPProblem& problem) const
{
    ILPSolution sol;

    std::vector<int> trueLiterals;
    for (int id : data.ppVarIDs) {
        int val = ipamir_val_lit(solver, id);
        trueLiterals.push_back(val > 0 ? id : -id);
    }

    std::vector<int> model = maxpre.reconstruct(trueLiterals);
    
    sol.feasible = true;
    sol.objValue = static_cast<double>(ipamir_val_obj(solver));
    sol.solValues.resize(problem.nVars(), 0.0);
    for (int i = 0; i < problem.nVars(); i++) {  
        sol.solValues[i] = (model[i] > 0) ? 1.0 : 0.0;
    }

    return sol;
}
#endif

} // namespace solver