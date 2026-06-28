#include "IncrementalMAFSolver.hpp"
#include "../Action/DeleteEdgeAction.hpp"
#include "../../Graph/Node.hpp"
#include "TreeUtils.hpp"

// Solver-Header...
#include "Interfaces/IncrUWrMaxSATSolver.hpp"

#include <cassert>
#include <stdexcept> 
#include <iostream>
#include <algorithm>
#include <chrono>


namespace solver {

// Constructor...
IncrementalMAFSolver::IncrementalMAFSolver(const std::shared_ptr<graph::Instance>& instance)
    : AbstractSolver(instance)
{
    forest_1 = (*instance)[0];
    forest_2 = (*instance)[1];
    lca1 = std::make_shared<cluster::LeastCommonAncestor>(forest_1);
    lca2 = std::make_shared<cluster::LeastCommonAncestor>(forest_2);
    nodeToIndex1 = buildNodeToIndexMap(*forest_1);
    nodeToIndex2 = buildNodeToIndexMap(*forest_2);
    labelToTerminal1 = buildLabelToTerminal(*forest_1);
    labelToTerminal2 = buildLabelToTerminal(*forest_2);
    cnt_constraints = 0;
}

IncrementalMAFSolver::IncrementalMAFSolver(const std::shared_ptr<graph::Instance>& instance,
                                           const std::shared_ptr<solver::Context>& context) : 
        AbstractSolver(instance),
        context(context)
{
    forest_1 = (*instance)[0];
    forest_2 = (*instance)[1];
    lca1 = std::make_shared<cluster::LeastCommonAncestor>(forest_1);
    lca2 = std::make_shared<cluster::LeastCommonAncestor>(forest_2);
    nodeToIndex1 = buildNodeToIndexMap(*forest_1);
    nodeToIndex2 = buildNodeToIndexMap(*forest_2);
    labelToTerminal1 = buildLabelToTerminal(*forest_1);
    labelToTerminal2 = buildLabelToTerminal(*forest_2);
    cnt_constraints = 0;
}

// =============================================================================
// solve
// =============================================================================
bool IncrementalMAFSolver::solve()
{   
    auto start = std::chrono::high_resolution_clock::now();

    // Important Variables:
    int cnt_rounds = 1;
    int numVars = getNumVars(*forest_1);

    // Inititalise Solver for first round...
    buildSolver(MaxSATSolverType::UWrMaxSAT);
    addInitialConstraints();

    while (true)
    {
        ILPSolution sol = solver->solve(numVars);
        auto cutEdges = extractCutEdges(sol);
        auto mafSolution = reconstructMAF(cutEdges, false);

        auto timepoint = std::chrono::high_resolution_clock::now();
        double timeSec = std::chrono::duration_cast<std::chrono::duration<double>>(timepoint - start).count();
        std::cout << "#r time passed: " << timeSec << "s \n" << std::endl;

        ++cnt_rounds;
        std::cout << "#r Round: " << cnt_rounds << std::endl;

        if (checkMAF(mafSolution))
        {
            std::cout << "#r end \n \n" << "==================================== \n" << std::endl;

            reconstructMAF(cutEdges, true);

            return true;
        }
        
        // Trick: As the IPAMIR-API gives back faulty values, we reninitialise the solver every round, with the collected constraints...
        buildSolver(MaxSATSolverType::UWrMaxSAT);
        for (auto constraint : constraints)
        {
            solver->addHardClause(constraint, true);
        }
    }
}


void IncrementalMAFSolver::buildSolver(MaxSATSolverType solverType)
{
    // Build & Initialise Solver...
    switch(solverType)
    {
        case MaxSATSolverType::UWrMaxSAT:
            solver = std::make_unique<IncrUWrMaxSATSolver>();
            break;

        // case MaxSATSolverType::EvalMaxSAT:
        //     solver = std::make_unique<IncrEvalMaxSATSolver>();
        //     break;
        
        default:
            throw std::invalid_argument("IncrementalMAFSolver: Unknown ILP solver type");
    }

    solver->initSolver(nodeToIndex1.size());

    // Add Soft Constraints...
    if (context)
    {
        unsigned int rootLabel = context->clusterRootLabel;
        if (rootLabel != 0)
        {
            int rootIndex = nodeToIndex1.at(labelToTerminal1.at(rootLabel));
            for(const auto& [_, index] : nodeToIndex1) 
            {
                if (index == rootIndex) {
                    solver->addSoftClause(index, 0.5); 
                } 
                else 
                {
                    solver->addSoftClause(index, 1.0);  
                }
            }
            return;
        }
    }
    
    for(const auto& [_, index] : nodeToIndex1) 
    {
        solver->addSoftClause(index, 1.0);
    }

    // Add Root-Constraint...
    int rootIndex = getRootIndex(*forest_1);
    std::vector<int> varIndices = { rootIndex };
    solver->addHardClause(varIndices, false);
    cnt_constraints++;
    
}

bool IncrementalMAFSolver::checkMAF(std::shared_ptr<graph::Forest>& mafSolution)
{
    int n_constraints = 0;
    bool pathpair = false;
    std::shared_ptr<cluster::LeastCommonAncestor> lca_sol = std::make_shared<cluster::LeastCommonAncestor>(mafSolution);
    std::unordered_map<const graph::Node*, int> nodeToIndex_sol = buildNodeToIndexMap(*mafSolution);
    std::unordered_map<unsigned int, graph::Node*> labelToTerminal_sol = buildLabelToTerminal(*mafSolution);

    for (const graph::Node* solutionRoot : mafSolution->Roots())
    {
        std::vector<unsigned int> labels = getSubtreeLabels(solutionRoot, mafSolution);
        std::sort(labels.begin(), labels.end());

        // Check if there are still constraints to be added...
        if (labels.size() < 3)
        {
            continue;
        }

        n_constraints = checkTripleConstraints(n_constraints, labels, false, mafSolution, lca_sol, nodeToIndex_sol, labelToTerminal_sol); 
        if (n_constraints < 0)
        {
            std::cout << "#r constraints: " << MAX_CONSTRAINTS_PER_ROUND << std::endl;
            return false;
        }

        // pathpair = true;
        // n_constraints = checkPathPairConstraints(n_constraints, true, mafSolution, lca_sol, nodeToIndex_sol, labelToTerminal_sol);
        // if (n_constraints < 0)
        // {
        //     std::cout << "#r constraints: " << MAX_CONSTRAINTS_PER_ROUND << std::endl;
        //     std::cout << "#r Pathpair constraints used! " << std::endl;
        //     return false;
        // }
    }

    if (n_constraints == 0)
        {
            pathpair = true;
            std::cout << "No more triple constraints - checking pathpair constraints..." << std::endl;
            n_constraints = checkPathPairConstraints(n_constraints, false, mafSolution, lca_sol, nodeToIndex_sol, labelToTerminal_sol);
            if (n_constraints < 0)
            {
                std::cout << "#r constraints: " << MAX_CONSTRAINTS_PER_ROUND << std::endl;
                std::cout << "#r Pathpair constraints used! " << std::endl;
                return false;
            }
            
        }

        if (n_constraints > 0)
        {
            std::cout << "#r constraints: " << n_constraints << std::endl;
            if (pathpair)
                std::cout << "#r Pathpair constraints used! " << std::endl;
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
    std::cout << "#r Round: 1" << std::endl;
    int n_constraints = 0;
    std::vector<unsigned int> labels;
    for (const auto& [_, label] : (*forest_1).TerminalToLabel())
    {
        labels.push_back(label);
    }
    std::sort(labels.begin(), labels.end());

    int numLeaves = labels.size();
    for (int i = 0; i + 2 < numLeaves; i += 3)
    {   
        if (checkIncompatibleTriple(labels[i], labels[i+1], labels[i+2], (*forest_1), (*forest_2), 
                                                        (*lca1), (*lca2), nodeToIndex1, nodeToIndex2, labelToTerminal1, labelToTerminal2))
        {
            generateTripleConstraint(labels[i], labels[i+1], labels[i+2]);
            n_constraints++;
        } 
    }

    for (int i = 0; i + 3 < numLeaves; i += 4)
    {   
        if (labels[i] == labels[i+1] || labels[i] == labels[i+3] || labels[i+2] == labels[i+1] || labels[i+2] == labels[i+3]) continue;
        {
            if(!areTwoPathsDisjoint((*forest_1), labels[i], labels[i+1], labels[i+2], labels[i+3], (*lca1), nodeToIndex1, labelToTerminal1)) continue;
            if( areTwoPathsDisjoint((*forest_2), labels[i], labels[i+1], labels[i+2], labels[i+3], (*lca2), nodeToIndex2, labelToTerminal2)) continue;

            generatePathPairConstraint(labels[i], labels[i+1], labels[i+2], labels[i+3]);
            n_constraints++; 
        }
    }
    std::cout << "#r constraints: " << n_constraints << std::endl;
    std::cout << "#r Pathpair constraints used! " << std::endl;
}

int IncrementalMAFSolver::checkTripleConstraints(int n_constraints, std::vector<unsigned int> labels, bool linear,
                                                std::shared_ptr<graph::Forest>& mafSolution,
                                                std::shared_ptr<cluster::LeastCommonAncestor>& lca_sol,
                                                std::unordered_map<const graph::Node*, int>& nodeToIndex_sol,
                                                std::unordered_map<unsigned int, graph::Node*>& labelToTerminal_sol)
{
    if (linear)
    {
        for (int i = 0; i + 2 < labels.size(); i += 3)
        {   
            if (checkIncompatibleTriple(labels[i], labels[i+1], labels[i+2], (*mafSolution), (*forest_2), (*lca_sol), (*lca2), 
                                                        nodeToIndex_sol, nodeToIndex2, labelToTerminal_sol, labelToTerminal2))
            {
                generateTripleConstraint(labels[i], labels[i+1], labels[i+2]);
                n_constraints++;
            } 
        }
    }
    else 
    {
        for(unsigned int i = 0; i < labels.size(); ++i)
        {
            for(unsigned int j = i+1; j < labels.size(); ++j)
            {
                for(unsigned int k = j+1; k < labels.size(); ++k)
                {
                    if (checkIncompatibleTriple(labels[i], labels[j], labels[k], (*mafSolution), (*forest_2), (*lca_sol), (*lca2), 
                                                        nodeToIndex_sol, nodeToIndex2, labelToTerminal_sol, labelToTerminal2))
                    {
                        generateTripleConstraint(labels[i], labels[j], labels[k]);
                        n_constraints++;
                        if (n_constraints >= MAX_CONSTRAINTS_PER_ROUND)
                            return -1;
                    }
                }
            }
        }  
    }  
    return n_constraints;
}

int IncrementalMAFSolver::checkPathPairConstraints(int n_constraints, bool linear,
                                                std::shared_ptr<graph::Forest>& mafSolution,
                                                std::shared_ptr<cluster::LeastCommonAncestor>& lca_sol,
                                                std::unordered_map<const graph::Node*, int>& nodeToIndex_sol,
                                                std::unordered_map<unsigned int, graph::Node*>& labelToTerminal_sol)
{
   std::vector<LeafPair> pairs = generateLeafPairs(mafSolution);

   if (linear)
   {
        for (unsigned int i = 0; i+1 < pairs.size(); i += 2)
        {
            const LeafPair& pair1 = pairs[i];
            const LeafPair& pair2 = pairs[i+1];

            if (pair1.l == pair2.l || pair1.l == pair2.r || pair1.r == pair2.l || pair1.r == pair2.r) continue;
            // but not disjoint in forest2...
            if(!areTwoPathsDisjoint((*mafSolution), pair1.l, pair1.r, pair2.l, pair2.r, (*lca_sol), nodeToIndex_sol, labelToTerminal_sol)) continue;
            if( areTwoPathsDisjoint((*forest_2), pair1.l, pair1.r, pair2.l, pair2.r, (*lca2), nodeToIndex2, labelToTerminal2)) continue;

            generatePathPairConstraint(pair1.l, pair1.r, pair2.l, pair2.r);
            n_constraints++;
            if (n_constraints >= MAX_CONSTRAINTS_PER_ROUND)
                return -1;
        }
   }
   else 
   {
    for (unsigned int i = 0; i < pairs.size(); ++i)
        {
            for (unsigned int j = i+1; j < pairs.size(); ++j)
            {
                const LeafPair& pair1 = pairs[i];
                const LeafPair& pair2 = pairs[j];
                
                if (pair1.l == pair2.l || pair1.l == pair2.r || pair1.r == pair2.l || pair1.r == pair2.r) continue;
                if(!areTwoPathsDisjoint((*mafSolution), pair1.l, pair1.r, pair2.l, pair2.r, (*lca_sol), nodeToIndex_sol, labelToTerminal_sol)) continue;
                if( areTwoPathsDisjoint((*forest_2), pair1.l, pair1.r, pair2.l, pair2.r, (*lca2), nodeToIndex2, labelToTerminal2)) continue;

                generatePathPairConstraint(pair1.l, pair1.r, pair2.l, pair2.r);
                n_constraints++;
                if (n_constraints >= MAX_CONSTRAINTS_PER_ROUND)
                    return -1;

            }
        }
   }
    return n_constraints;
}


void IncrementalMAFSolver::generateTripleConstraint(unsigned int label1, unsigned int label2, unsigned int label3)
{
    std::vector<int> edgesij = getPath((*forest_1), label1, label2, (*lca1), nodeToIndex1, labelToTerminal1);
    std::vector<int> edgesjk = getPath((*forest_1), label2, label3, (*lca1), nodeToIndex1, labelToTerminal1);
    std::vector<int> edgesik = getPath((*forest_1), label1, label3, (*lca1), nodeToIndex1, labelToTerminal1);

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
    cnt_constraints++;
}

void IncrementalMAFSolver::generatePathPairConstraint(unsigned int lpair1, unsigned int rpair1, 
                                                      unsigned int lpair2, unsigned int rpair2)
{
    std::vector<int> edgesij = getPath((*forest_1), lpair1, rpair1, (*lca1), nodeToIndex1, labelToTerminal1);
    std::vector<int> edgespq = getPath((*forest_1), lpair2, rpair2, (*lca1), nodeToIndex1, labelToTerminal1);

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
    cnt_constraints++;
}

std::vector<IncrementalMAFSolver::LeafPair> IncrementalMAFSolver::generateLeafPairs(std::shared_ptr<graph::Forest>& mafSolution)
{
    std::vector<LeafPair> pairs;

    for (auto root : mafSolution->Roots())
    {
        // Leaves dieses Subtrees sammeln
        std::vector<unsigned int> subtreeLabels = getSubtreeLabels(root, mafSolution);
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

void recGetSubtreeLabels(const graph::Node* n, std::vector<unsigned int>& labels, std::shared_ptr<graph::Forest>& forest)
{
    if (!n) return;
    if (!n->leftChild)  // Blatt = kein linkes Kind
        labels.push_back(forest->TerminalToLabel().at(const_cast<graph::Node*>(n)));
    recGetSubtreeLabels(n->leftChild, labels, forest);
    recGetSubtreeLabels(n->rightChild, labels, forest);
}
std::vector<unsigned int> IncrementalMAFSolver::getSubtreeLabels(const graph::Node* subtreeRoot, std::shared_ptr<graph::Forest>& forest)
{
    auto labels = std::vector<unsigned int>();
    recGetSubtreeLabels(subtreeRoot, labels, forest);
    return labels;

}

}  // namespace solver