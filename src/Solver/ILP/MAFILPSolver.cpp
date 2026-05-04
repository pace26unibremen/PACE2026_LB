#include "MAFILPSolver.hpp"
#include "../Action/DeleteEdgeAction.hpp"
#include "TreeUtils.hpp"

// Solver-Header...
#ifdef USE_EVALMAXSAT
#include "Interfaces/EvalMaxSATSolver.hpp"
#endif
#ifdef USE_SCIP
#include "Interfaces/SCIPSolver.hpp"
#endif
#ifdef USE_UWRMAXSAT
#include "Interfaces/UWrMaxSatSolver.hpp"
#endif

#include <cassert>
#include <stdexcept> 
#include <iostream>

// TODO: Include concrete solver headers once implemented

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
std::shared_ptr<graph::Forest> MAFILPSolver::solve()
{
    // TODO: Early-exit if no constraints → trees already compatible, SPR-distance = 0
    // If problem.nConstraints() == 0, skip solver and return reconstructMAF({})
    // This avoids calling the ILP solver for trivial instances.
    
    ILPProblem problem = buildProblem();
    ILPSolution solution        = solveProblem(problem);
    std::vector<int> cutEdges   = extractCutEdges(solution);
    return reconstructMAF(cutEdges);
}


ILPProblem MAFILPSolver::buildProblem() const
{
    // Instance is a vector of forests...
    // NOTE: Currently ILP Formulation is only valid for binary MAF-Problem...
    // DEFAULT: Take first two forests as inputs...
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

std::shared_ptr<graph::Forest> MAFILPSolver::reconstructMAF(const std::vector<int>& cutEdges) const
{
    // Copy of Forest...
    graph::Forest forest = (*instance)[0]->copy();
    std::shared_ptr<graph::Forest> forestPtr = std::make_shared<graph::Forest>(forest);

    // Build index map on original forest, to match edges of vector-list...
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

    return forestPtr;
}

std::unique_ptr<AbstractILPSolver> MAFILPSolver::createSolver(ILPSolverType solverType)
{
    switch(solverType)
    {
        // TODO: Uncomment once concrete solvers are implemented
        case ILPSolverType::SCIP:
            #ifdef USE_SCIP
                return std::make_unique<SCIPSolver>();
            #else
                throw std::runtime_error("SCIP not available - rebuild with USE_SCIP");
            #endif
        case ILPSolverType::EvalMaxSAT: 
            #ifdef USE_EVALMAXSAT
                return std::make_unique<EvalMaxSATSolver>();
            #else
                throw std::runtime_error("EvalMaxSATSolver not available - rebuild with USE_EVALMAXSAT");
            #endif
        case ILPSolverType::UWrMaxSat: 
            #ifdef USE_UWRMAXSAT
                return std::make_unique<UWrMaxSatSolver>();
            #else
                throw std::runtime_error("EvalMaxSATSolver not available - rebuild with USE_UWRMAXSAT");
            #endif
        default:
            throw std::invalid_argument("MAFILPSolver: Unknown ILP solver type");
    }
}

}  // namespace solver