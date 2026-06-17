#include "IncrementalMAFSolver.hpp"
#include "../Action/DeleteEdgeAction.hpp"
#include "../../Cluster/LeastCommonAncestor.hpp"
#include "../../Graph/Forest.hpp"
#include "../../Graph/ForestIO.hpp"
#include "../../Graph/Node.hpp"
#include "../Rule/SubtreeReductionRule.hpp"
#include "../Context.hpp"
#include "TreeUtils.hpp"
#include "TimerUtils.hpp"

// Solver-Header...
#include "Interfaces/IncrUWrMaxSATSolver.hpp"

#include <cassert>
#include <stdexcept> 
#include <iostream>


namespace solver {

// Constructor...
IncrementalMAFSolver::IncrementalMAFSolver(const std::shared_ptr<graph::Instance>& instance,
                             MaxSATSolverType solverType)
    : AbstractSolver(instance)
{
    forest_1 = (*instance)[0];
    forest_2 = (*instance)[1];

    lca1 = std::make_shared<cluster::LeastCommonAncestor>(forest_1);
    lca2 = std::make_shared<cluster::LeastCommonAncestor>(forest_2);
    nodeToIndex1 = buildNodeToIndexMap(*forest_1);
    nodeToIndex2 = buildNodeToIndexMap(*forest_2);
    buildSolver(solverType);
}

// =============================================================================
// solve
// =============================================================================
bool IncrementalMAFSolver::solve()
{   
    addInitialConstraints();
    int cnt = 0;
    int numVars = forest_1->Nodes().size();
    //forest_2->dot("debug/debug_forest_2_error.dot");
    while (true)
    {
        ILPSolution sol = solver->solve(numVars);
        auto cutEdges = extractCutEdges(sol);
    
        // Debug-Ausgabe...
        //auto indexToNode = buildIndexToNodeMap(*forest_1);
        //forest_1->dotMaxSAT("debug/debug_round_" + std::to_string(cnt) + "_error.dot", cutEdges, indexToNode);
        auto mafSolution = reconstructMAF(cutEdges, false);
       //mafSolution->dot("debug/debug_solution_round_" + std::to_string(cnt) + "_error.dot");
    
        if (checkMAF(mafSolution))
        {
            reconstructMAF(cutEdges, true);
            return true;
        }
        
        buildSolver(MaxSATSolverType::UWrMaxSAT);
        for (auto constraint : constraints)
        {
            solver->addHardClause(constraint, true);
        }
        
        ++cnt;
        if (cnt > MAX_ROUNDS) return false;
    }
}


void IncrementalMAFSolver::buildSolver(MaxSATSolverType solverType)
{
    // Build & Initialise Solver...
    switch(solverType)
    {
        case MaxSATSolverType::UWrMaxSAT:
            solver = std::make_unique<IncrUWrMaxSatSolver>();
            break;
        default:
            throw std::invalid_argument("IncrementalMAFSolver: Unknown ILP solver type");
    }

    solver->initSolver();

    // Add Soft Constraints...
    int numVars = forest_1->Nodes().size();
    for(int i = 0; i < numVars; i++) 
    {
        solver->addSoftClause(i, 1.0);
    }

    // Add Root-Constraint...
    int rootIndex = getRootIndex(*forest_1);
    std::vector<int> varIndices = { rootIndex };
    solver->addHardClause(varIndices, false);
}

bool IncrementalMAFSolver::checkMAF(std::shared_ptr<graph::Forest>& mafSolution)
{
    int n_constraints = 0;
    std::shared_ptr<cluster::LeastCommonAncestor> lca_sol = std::make_shared<cluster::LeastCommonAncestor>(mafSolution);
    std::unordered_map<const graph::Node*, int> nodeToIndex_sol = buildNodeToIndexMap(*mafSolution);

    for (const graph::Node* solutionRoot : mafSolution->Roots())
    {
        std::vector<unsigned int> labels = getSubtreeLabels(solutionRoot);

        // Check if there are still constraints to be added...
        int numLeaves = labels.size();
        if (numLeaves < 3)
        {
            continue;
        }

        if (numLeaves < 100)
        {
            n_constraints = checkTripleConstraints(numLeaves, n_constraints, labels, mafSolution, lca_sol, nodeToIndex_sol); 
            if (n_constraints < 0)
            {
                return false;
            }  
        }
        else
        {
            for (int i = 0; i + 2 < numLeaves; i += 3)
            {   
                if (checkIncompatibleTriple(labels[i], labels[i+1], labels[i+2], (*mafSolution), (*forest_2), (*lca_sol), (*lca2), nodeToIndex_sol, nodeToIndex2))
                {
                    generateTripleConstraint(labels[i], labels[i+1], labels[i+2]);
                    n_constraints++;
                    if (n_constraints < 0)
                    {
                        return false;
                    }
                } 
            }
        }
    }

    if (n_constraints == 0)
    {
        n_constraints = checkPathPairConstraints(n_constraints, mafSolution, lca_sol, nodeToIndex_sol);
        if (n_constraints < 0)
        {
            return false;
        }
    }

    if (n_constraints > 0)
    {
        return false;
    }

    return true;
}

std::vector<int> IncrementalMAFSolver::extractCutEdges(const ILPSolution& solution)
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

std::shared_ptr<graph::Forest> IncrementalMAFSolver::reconstructMAF(std::vector<int> cutEdges, bool orig)
{

    auto forestPtr = (*instance)[0];
    if (!orig)
    {
        graph::Forest forest = (*instance)[0]->copy();
        forestPtr = std::make_shared<graph::Forest>(forest);
    }
    
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

void IncrementalMAFSolver::addInitialConstraints()
{
    std::vector<unsigned int> labels;
    for (const auto& [_, label] : (*forest_1).TerminalToLabel())
    {
        labels.push_back(label);
    }
    std::sort(labels.begin(), labels.end());

    int numLeaves = labels.size();
    for (int i = 0; i + 2 < numLeaves; i += 3)
    {   
        if (checkIncompatibleTriple(labels[i], labels[i+1], labels[i+2], (*forest_1), (*forest_2), (*lca1), (*lca2), nodeToIndex1, nodeToIndex2))
        {
            generateTripleConstraint(labels[i], labels[i+1], labels[i+2]);
        } 
    }
}

int IncrementalMAFSolver::checkTripleConstraints(int numLeaves, int n_constraints, std::vector<unsigned int> labels,
                                                std::shared_ptr<graph::Forest>mafSolution,
                                                std::shared_ptr<cluster::LeastCommonAncestor> lca_sol,
                                                std::unordered_map<const graph::Node*, int> nodeToIndex_sol)
{
    for(unsigned int i = 0; i < numLeaves; ++i)
            {
                for(unsigned int j = i+1; j < numLeaves; ++j)
                {
                    for(unsigned int k = j+1; k < numLeaves; ++k)
                    {
                        if (checkIncompatibleTriple(labels[i], labels[j], labels[k], (*mafSolution), (*forest_2), (*lca_sol), (*lca2), nodeToIndex_sol, nodeToIndex2))
                        {
                            generateTripleConstraint(labels[i], labels[j], labels[k]);
                            n_constraints++;
                            if (n_constraints >= MAX_CONSTRAINTS_PER_ROUND)
                                return -1;
                        }
                    }
                }
            }    
    return n_constraints;
}

int IncrementalMAFSolver::checkPathPairConstraints(int n_constraints,
                                                std::shared_ptr<graph::Forest>mafSolution,
                                                std::shared_ptr<cluster::LeastCommonAncestor> lca_sol,
                                                std::unordered_map<const graph::Node*, int> nodeToIndex_sol)
{
   std::vector<LeafPair> pairs = generateLeafPairs(mafSolution);
   for (unsigned int i = 0; i < pairs.size(); ++i)
    {
        for (unsigned int j = i+1; j < pairs.size(); ++j)
        {
            const LeafPair& pair1 = pairs[i];
            const LeafPair& pair2 = pairs[j];

            if (pair1.l == pair2.l || pair1.l == pair2.r || pair1.r == pair2.l || pair1.r == pair2.r) continue;
            // but not disjoint in forest2...
            if(!areTwoPathsDisjoint((*mafSolution), pair1.l, pair1.r, pair2.l, pair2.r, (*lca_sol), nodeToIndex_sol)) continue;
            if( areTwoPathsDisjoint((*forest_2), pair1.l, pair1.r, pair2.l, pair2.r, (*lca2), nodeToIndex2)) continue;

            generatePathPairConstraint(pair1.l, pair1.r, pair2.l, pair2.r);
            n_constraints++;
            if (n_constraints >= MAX_CONSTRAINTS_PER_ROUND)
                return -1;

        }
    }
    return n_constraints;
}


void IncrementalMAFSolver::generateTripleConstraint(unsigned int label1, unsigned int label2, unsigned int label3)
{
    std::vector<int> edgesij = getPath((*forest_1), label1, label2, (*lca1), nodeToIndex1);
    std::vector<int> edgesjk = getPath((*forest_1), label2, label3, (*lca1), nodeToIndex1);
    std::vector<int> edgesik = getPath((*forest_1), label1, label3, (*lca1), nodeToIndex1);

    // Union of all paths...
    std::vector<int> triPathEdges;
    triPathEdges.reserve(edgesij.size() + edgesjk.size() + edgesik.size());
    triPathEdges.insert(triPathEdges.end(), edgesij.begin(), edgesij.end());
    triPathEdges.insert(triPathEdges.end(), edgesjk.begin(), edgesjk.end());
    triPathEdges.insert(triPathEdges.end(), edgesik.begin(), edgesik.end());
    
    // Make Sure the union of set is unique...
    std::sort(triPathEdges.begin(), triPathEdges.end());
    triPathEdges.erase(
        std::unique(triPathEdges.begin(), triPathEdges.end()),
        triPathEdges.end()
    );
    
    solver->addHardClause(triPathEdges, true);
    constraints.push_back(triPathEdges);
}

void IncrementalMAFSolver::generatePathPairConstraint(unsigned int lpair1, unsigned int rpair1, 
                                                      unsigned int lpair2, unsigned int rpair2)
{
    std::vector<int> edgesij = getPath((*forest_1), lpair1, rpair1, (*lca1), nodeToIndex1);
    std::vector<int> edgespq = getPath((*forest_1), lpair2, rpair2, (*lca1), nodeToIndex1);

    std::vector<int> pathPairEdges;
    pathPairEdges.reserve(edgesij.size() + edgespq.size());
    pathPairEdges.insert(pathPairEdges.end(), edgesij.begin(), edgesij.end());
    pathPairEdges.insert(pathPairEdges.end(), edgespq.begin(), edgespq.end());

    std::sort(pathPairEdges.begin(), pathPairEdges.end());
    pathPairEdges.erase(
        std::unique(pathPairEdges.begin(), pathPairEdges.end()),
        pathPairEdges.end()
    );
    solver->addHardClause(pathPairEdges, true);
    constraints.push_back(pathPairEdges);
}

std::vector<IncrementalMAFSolver::LeafPair> IncrementalMAFSolver::generateLeafPairs(std::shared_ptr<graph::Forest>& mafSolution)
{
    std::vector<LeafPair> pairs;

    for (auto root : mafSolution->Roots())
    {
        // Leaves dieses Subtrees sammeln
        std::vector<unsigned int> subtreeLabels = getSubtreeLabels(root);
        int n = subtreeLabels.size();

        // n=1: ignorieren
        if (n <= 1) continue;

        // n=2: genau ein Paar
        if (n == 2)
        {
            pairs.push_back({subtreeLabels[0], subtreeLabels[1]});
            continue;
        }

        // n>2: alle internen Paare
        for (int i = 0; i < n; ++i)
            for (int j = i+1; j < n; ++j)
                pairs.push_back({subtreeLabels[i], subtreeLabels[j]});
    }

    return pairs;
}

std::vector<unsigned int> IncrementalMAFSolver::getSubtreeLabels(const graph::Node* subtreeRoot)
{
    int k = 0;
    std::vector<unsigned int> labels;
    for (uint64_t bitmask : subtreeRoot->subtreeTerminals)
    {
        while (bitmask != 0)
        {
            unsigned i = __builtin_ctzll(bitmask);
            bitmask &= bitmask - 1;
            unsigned int label = 64 * k + i + 1;
            labels.push_back(label);
        }
        k++;
    }

    return labels;
}

}  // namespace solver