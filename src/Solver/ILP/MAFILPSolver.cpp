#include "MAFILPSolver.hpp"
#include "../Action/DeleteEdgeAction.hpp"
#include "../Rule/SubtreeReductionRule.hpp"
#include "../Context.hpp"
#include "TreeUtils.hpp"

// Solver-Header...
#include "Interfaces/UWrMaxSatSolver.hpp"

#include <cassert>
#include <stdexcept> 
#include <iostream>

namespace solver {

// Constructor...
MAFILPSolver::MAFILPSolver(const std::shared_ptr<graph::Instance>& instance,
                             ILPSolverType solverType)
    : AbstractSolver(instance)
{
    ilpSolver = createSolver(solverType);
}

// =============================================================================
// solve
// =============================================================================
bool MAFILPSolver::solve()
{
    ILPProblem problem = buildProblem();
    ILPSolution solution = solveProblem(problem);

    if (!solution.feasible) 
        return false;
    std::vector<int> cutEdges   = extractCutEdges(solution);
    reconstructMAF(cutEdges);

    return true;
}


ILPProblem MAFILPSolver::buildProblem() const
{
    // ILP Formulation is only valid for binary MAF-Problem...
    assert(instance->size() == 2);

    ILPFormulation formulation((*instance)[0], (*instance)[1]);
    return formulation.build();
}

ILPSolution MAFILPSolver::solveProblem(const ILPProblem problem) const
{
    return ilpSolver->solve(problem);
}


std::vector<int> MAFILPSolver::extractCutEdges(const ILPSolution& solution) const
{
    std::vector<int> cutEdges;

    if(!solution.feasible)
        return cutEdges;

    for(int i = 0; i < (int)solution.solValues.size(); ++i)
    {
        if(solution.solValues[i] > 0.5)
            cutEdges.push_back(i);
    }
    return cutEdges;
}

void MAFILPSolver::reconstructMAF(const std::vector<int>& cutEdges)
{
    // Build index map on original forest, to match edges of vector-list...
    auto forestPtr = (*instance)[0];
    auto indexToNode = buildIndexToNodeMap(*forestPtr);

    // Apply Cut Edges...
    for (int edgeIndex : cutEdges)
    {
        // Edge (Node) should be available...
        assert(indexToNode.count(edgeIndex) > 0.5);
        graph::Node* child = indexToNode.at(edgeIndex);
        DeleteEdgeAction action(child, forestPtr);
        action.doAction();
    }
}

std::unique_ptr<AbstractILPSolver> MAFILPSolver::createSolver(ILPSolverType solverType)
{
    switch(solverType)
    {
        // TODO: Uncomment once concrete solvers are implemented
        // case ILPSolverType::SCIP:
        //     return std::make_unique<SCIPSolver>();
    
        // case ILPSolverType::EvalMaxSAT: 
        //     return std::make_unique<EvalMaxSATSolver>();
            
        case ILPSolverType::UWrMaxSat: 
            return std::make_unique<UWrMaxSatSolver>();

        default:
            throw std::invalid_argument("MAFILPSolver: Unknown ILP solver type");
    }
}

}  // namespace solver