#include "MAFILPSolver.hpp"
#include "../Action/DeleteEdgeAction.hpp"
#include "../Rule/SubtreeReductionRule.hpp"
#include "../Context.hpp"
#include "TreeUtils.hpp"
#include "TimerUtils.hpp"

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
bool MAFILPSolver::solve()
{
    // TODO: Early-exit if no constraints → trees already compatible, SPR-distance = 0
    // If problem.nConstraints() == 0, skip solver and return reconstructMAF({})
    // This avoids calling the ILP solver for trivial instances.
    
    // Apply Subtree Reduction Rule...
    std::shared_ptr<Context> context = std::make_shared<Context>();
    std::list<std::shared_ptr<AbstractRule>> appliedReductions;
    auto subtreeReduction = solver::SubtreeReductionRule::isApplicable(instance, context);
    if (subtreeReduction)
    {
        std::clog << "Subtree Reduction is applicable!" << std::endl;
        subtreeReduction->apply();
    }

    TIMER_START(t_build)
    ILPProblem problem = buildProblem();
    TIMER_LOG(t_build, "MAFILPSOLVER::build_problem")

    std::clog << "Number of Variables in Problem:" << problem.nVars() << std::endl;
    std::clog << "Number of Constraints in Problem:" << problem.nConstraints() << std::endl;
    
    TIMER_START(t_solve)
    ILPSolution solution        = solveProblem(problem);
    TIMER_LOG(t_solve, "MAFILPSOLVER::solve_problem")

    if (!solution.feasible) 
        return false;
    std::vector<int> cutEdges   = extractCutEdges(solution);
    reconstructMAF(cutEdges);

    
    subtreeReduction->unapply();

    return true;
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
                throw std::runtime_error("UWrMaxSatSolver not available - rebuild with USE_UWRMAXSAT");
            #endif
        default:
            throw std::invalid_argument("MAFILPSolver: Unknown ILP solver type");
    }
}

}  // namespace solver