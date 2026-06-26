#include "IncrEvalMaxSATSolver.hpp"
#include <cassert>
#include <iostream>
#include <chrono>

namespace solver {

void IncrEvalMaxSATSolver::initSolver(int numVars)
{
    solver_evalmaxsat.setIncremental(true);
}

void IncrEvalMaxSATSolver::addSoftClause(int id, double objCoeff)
{
    // Variablen werden lazy erstellt - sicherstellen dass genug vorhanden
    varIDs[id] = solver_evalmaxsat.newVar();

    int weight = static_cast<int>(objCoeff + 0.5);
    solver_evalmaxsat.addClause({-varIDs[id]}, weight);
}

void IncrEvalMaxSATSolver::addHardClause(const std::vector<int>& varIndices, bool isLowerBound)
{
    if (isLowerBound)
    {
        std::vector<int> clause;
        for (int var : varIndices)
            clause.push_back(varIDs[var]);
        solver_evalmaxsat.addClause(clause);
    }
    else
    {
        for (int var : varIndices)
            solver_evalmaxsat.addClause({-varIDs[var]});
    }
}

void IncrUWrMaxSATSolver::addAssumptions(const std::vector<double>& solValues)
{
    throw std::runtime_error("IncrEvalmaxsatSolver: Doesn't suppport assumptions...");
}

ILPSolution IncrEvalMaxSATSolver::solve(int numVars)
{
    ILPSolution sol;

    auto start = std::chrono::high_resolution_clock::now();
    std::streambuf* oldBuf = std::cout.rdbuf(nullptr);
    bool sat = solver_evalmaxsat.solve();
    std::cout.rdbuf(oldBuf);
    auto end = std::chrono::high_resolution_clock::now();
    double timeSec = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();
    std::cout << "#r solve_time: " << timeSec << "s" << std::endl;

    if (!sat) return sol;

    sol.feasible = true;
    sol.objValue = static_cast<double>(solver_evalmaxsat.getCost());
    std::cout << "#r solve_score: " << sol.objValue + 1 << std::endl;
    sol.solValues.resize(numVars, 0.0);

    for (int i = 0; i < numVars; ++i)
    {
        auto it = varIDs.find(i);
        if (it != varIDs.end()) {
            sol.solValues[i] = solver_evalmaxsat.getValue(it->second) ? 1.0 : 0.0;
        } else {
            throw std::runtime_error("IncrEvalmaxsatSolver: Variable Index out of bounds...");
        }
    }

    return sol;
}

} // namespace solver