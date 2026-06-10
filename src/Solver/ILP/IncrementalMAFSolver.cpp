#include "IncrementalMAFSolver.hpp"
#include "../Action/DeleteEdgeAction.hpp"
#include "../../Graph/Forest.hpp"
#include "../../Graph/Node.hpp"
#include "../Rule/SubtreeReductionRule.hpp"
#include "../Context.hpp"
#include "TreeUtils.hpp"
#include "TimerUtils.hpp"

// Solver-Header...
#ifdef USE_EVALMAXSAT
#include "Interfaces/IncrEvalMaxSATSolver.hpp"
#endif
#ifdef USE_UWRMAXSAT
#include "Interfaces/IncrUWrMaxSatSolver.hpp"
#endif

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
    int numLeaves = forest_1->LabelToTerminal().size();
    pathCache1.reserve(numLeaves * (numLeaves - 1) / 2);
    pathCache2.reserve(numLeaves * (numLeaves - 1) / 2);
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

    while (true)
    {
        ILPSolution sol = solver->solve(numVars);
        extractCutEdges(sol);
        auto mafSolution = reconstructMAF(false);
        if (checkMAF(mafSolution))
        {
            reconstructMAF(true);
            return true;
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
        /*
        case MaxSATSolverType::EvalMaxSAT: 
            #ifdef USE_EVALMAXSAT
                solver = std::make_unique<EvalMaxSATSolver>();
            #else
                throw std::runtime_error("EvalMaxSATSolver not available - rebuild with USE_EVALMAXSAT");
            #endif
        */
        case MaxSATSolverType::UWrMaxSat: 
            #ifdef USE_UWRMAXSAT
                solver = std::make_unique<IncrUWrMaxSatSolver>();
            #else
                throw std::runtime_error("UWrMaxSatSolver not available - rebuild with USE_UWRMAXSAT");
            #endif
        default:
            throw std::invalid_argument("MAFILPSolver: Unknown ILP solver type");
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

bool IncrementalMAFSolver::checkMAF(const std::shared_ptr<graph::Forest>& mafSolution)
{
    bool isMAF = true;

    int numSubtrees = mafSolution->Roots().size();
    int bitmaskSize = (numSubtrees + 63) / 64;

    std::unordered_map<const graph::Node*, std::vector<uint64_t>> nodeToSubtrees;
    std::vector<bool> constrainedSubtrees(numSubtrees, false);

    int subtreeID = 0;
    for (const graph::Node* solutionRoot : mafSolution->Roots())
    {

        // Prüfe ob S in forest_2 einbettbar ist
        auto isSubtree = isSubtreeOfForest(solutionRoot, mafSolution, forest_2, subtreeID, bitmaskSize, nodeToSubtrees);
        if (!isSubtree)
        {
            isMAF = false;
            constrainedSubtrees[subtreeID] = true;
            generateConstraints({solutionRoot}, mafSolution);
        }
        subtreeID++;
    }

    for (const auto& [node, bitmask] : nodeToSubtrees)
    {
        // Sammle alle Subtree-IDs die diesen Knoten beanspruchen
        std::vector<const graph::Node*> overlappingSubtreeRoots;
        for (int i = 0; i < numSubtrees; i++)
        {
            if (bitmask[i / 64] & (1ULL << (i % 64)))
            {
                if (!constrainedSubtrees[i])
                    overlappingSubtreeRoots.push_back(mafSolution->Roots()[i]);
            }
        }
        
        if (overlappingSubtreeRoots.size() > 1)
        {
            isMAF = false;
            generateConstraints(overlappingSubtreeRoots, mafSolution);  // Constraint 2
        }
    }

    return isMAF;
}

void IncrementalMAFSolver::extractCutEdges(const ILPSolution& solution)
{

    if(!solution.feasible)
        return;

    for(int i = 0; i < (int)solution.solValues.size(); ++i)
    {
        if(solution.solValues[i] > 0.5)
            cutEdges.push_back(i);
    }

    return;
}

std::shared_ptr<graph::Forest>& IncrementalMAFSolver::reconstructMAF(bool orig)
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
    auto nodeToIndex1 = buildNodeToIndexMap(*forest_1);
    auto nodeToIndex2 = buildNodeToIndexMap(*forest_2);

    std::vector<unsigned int> labels;
    for (const auto& [_, label] : (*forest_1).TerminalToLabel())
    {
        labels.push_back(label);
    }
    std::sort(labels.begin(), labels.end());

    int numLeaves = labels.size();

    for (int i = 0; i + 2 < numLeaves; i + 3)
    {   
        if (checkIncompatibleTriple(labels[i], labels[i+1], labels[i+2], (*forest_1), (*forest_2), (*lca1), (*lca2), nodeToIndex1, nodeToIndex2))
        {
            std::vector<int>& edgesij = getPath((*forest_1), labels[i], labels[i+1], (*lca1), nodeToIndex1, pathCache1);
            std::vector<int>& edgesjk = getPath((*forest_1), labels[i+1], labels[i+2], (*lca1), nodeToIndex1, pathCache1);
            std::vector<int>& edgesik = getPath((*forest_1), labels[i], labels[i+2], (*lca1), nodeToIndex1, pathCache1);

            // Union of all paths...
            std::vector<int> triPathEdges;
            triPathEdges.reserve(edgesij.size() + edgesjk.size() + edgesik.size());
            triPathEdges.insert(triPathEdges.end(), edgesij.begin(), edgesij.end());
            triPathEdges.insert(triPathEdges.end(), edgesjk.begin(), edgesjk.end());
            triPathEdges.insert(triPathEdges.end(), edgesik.begin(), edgesik.end());
            
            // Make Sure it union of set is unique...
            std::sort(triPathEdges.begin(), triPathEdges.end());
            triPathEdges.erase(
                std::unique(triPathEdges.begin(), triPathEdges.end()),
                triPathEdges.end()
            );

            solver->addHardClause(triPathEdges, true);
        } 
    }
}

void IncrementalMAFSolver::generateConstraints(std::vector<const graph::Node*> subtreeRoots,
                                                const std::shared_ptr<graph::Forest>& mafSolution)
{
    std::vector<int> allEdges;
    
    for (const graph::Node* root : subtreeRoots)
    {
        int k = 0;
        for (uint64_t bitmask : root->subtreeTerminals)
        {
            while (bitmask != 0)
            {
                unsigned i = __builtin_ctzll(bitmask);
                bitmask &= bitmask - 1;

                unsigned int label = 64 * k + i + 1;
                
                // Starte synchrone Traversierung von Terminal nach oben
                const graph::Node* currentSub = mafSolution->LabelToTerminal().at(label);
                const graph::Node* currentOrig = forest_1->LabelToTerminal().at(label);
                
                // Gehe synchron nach oben bis zum Root des Subtrees in mafSolution
                while (currentSub != root)
                {
                    assert(nodeToIndex1.count(currentOrig) > 0);
                    allEdges.push_back(nodeToIndex1.at(currentOrig));
                    
                    currentSub = currentSub->parent;
                    currentOrig = currentOrig->parent;
                }
            }
            k++;
        }
    }
    
    // Remove Duplikates...
    std::sort(allEdges.begin(), allEdges.end());
    allEdges.erase(
        std::unique(allEdges.begin(), allEdges.end()),
        allEdges.end()
    );
    
    solver->addHardClause(allEdges, true);
}

bool IncrementalMAFSolver::isSubtreeOfForest(const graph::Node* subtreeRoot, 
                        const std::shared_ptr<graph::Forest>& mafSolution, 
                        const std::shared_ptr<graph::Forest>& forest,
                        int subtreeID,
                        int bitmaskSize,
                        std::unordered_map<const graph::Node*, std::vector<uint64_t>>& nodeToSubtrees)
{
    std::function<bool(graph::Node*, graph::Node*)> traverseUp = [&](const graph::Node* subtreePtr, const graph::Node* treePtr) -> bool {

        auto& bitmask = nodeToSubtrees.emplace(treePtr, std::vector<uint64_t>(bitmaskSize, 0)).first->second;
        bitmask[subtreeID / 64] |= (1ULL << (subtreeID % 64));
        
        if (not subtreePtr->hasSameTerminals(treePtr))
        {
            return false;
        }
        if (subtreePtr->parent == nullptr)
        {
            return true;
        }
        if (treePtr->parent == nullptr)
        {
            return false;
        }
        return traverseUp(subtreePtr->parent, treePtr->parent);
    };

    int k = 0;
    for(uint64_t bitmask : subtreeRoot->subtreeTerminals)
    {
        while (bitmask != 0) {
            unsigned i = __builtin_ctzll(bitmask);
            bitmask &= bitmask - 1;

            unsigned int label = 64 * k + i + 1;
            if (not traverseUp(mafSolution->LabelToTerminal()[label], forest->LabelToTerminal()[label]))
            {
                return false;
            }
        }
        k++;
    }   
    return true;        
}

}  // namespace solver