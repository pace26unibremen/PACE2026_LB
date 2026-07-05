#include "IncrementalMAFSolver.hpp"
#include "../Action/DeleteEdgeAction.hpp"
#include "../../Graph/Node.hpp"
#include "TreeUtils.hpp"

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
    forestData_1 = buildForestData((*instance)[0]);
    forestData_2 = buildForestData((*instance)[1]);
    cnt_constraints = 0;
    int numLeaves = forestData_1.labelToTerminal.size();
    MAX_CONSTRAINTS_PER_ROUND = numLeaves;
    MIN_CONSTRAINTS_PER_ROUND = numLeaves;
    MAX_CONSTRAINTS_PER_ROUND_PER_SUBTREE = numLeaves;
    currentLB = 0;

}

IncrementalMAFSolver::IncrementalMAFSolver(const std::shared_ptr<graph::Instance>& instance,
                                           const std::shared_ptr<solver::Context>& context) : 
        AbstractSolver(instance),
        context(context)
{
    forestData_1 = buildForestData((*instance)[0]);
    forestData_2 = buildForestData((*instance)[1]);
    cnt_constraints = 0;
    int numLeaves = forestData_1.labelToTerminal.size();
    MAX_CONSTRAINTS_PER_ROUND = numLeaves;
    MIN_CONSTRAINTS_PER_ROUND = numLeaves;
    MAX_CONSTRAINTS_PER_ROUND_PER_SUBTREE = numLeaves;
    currentLB = 0;
}

// =============================================================================
// solve
// =============================================================================
bool IncrementalMAFSolver::solve()
{   
    auto start = std::chrono::high_resolution_clock::now();
    auto future =  std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::time_point::duration>(std::chrono::duration<double>(timeOutDelay));

    // Important Variables:
    int cnt_rounds = 1;
    int numVars = getNumVars(*forestData_1.forestPtr);

    // Initialise Solver for first round...
    buildSolver();
    if (constraints.size() > 0)
    {
        for (auto constraint : constraints)
        {
            solver->addHardClause(constraint, true);
        }
    }
    else
    {
        addInitialConstraints();
    }

    while (true)
    {
        if (future < std::chrono::steady_clock::now())
            return false;

        ILPSolution sol = solver->solve(numVars);
        if (!sol.feasible)
            return false;
        currentLB = sol.objValue + 1;

        auto cutEdges = extractCutEdges(sol);
        auto mafSolution = reconstructMAF(cutEdges, false);

        ++cnt_rounds;
        MAX_CONSTRAINTS_PER_ROUND = std::min(static_cast<int>(10*MIN_CONSTRAINTS_PER_ROUND), static_cast<int> (MAX_CONSTRAINTS_PER_ROUND * (0.1 * cnt_rounds)));
        
        if (checkMAF(mafSolution))
        {
            std::cout << "#r end \n \n" << "==================================== \n" << std::endl;
            reconstructMAF(cutEdges, true);
            return true;
        }
        
        buildSolver();
        for (auto constraint : constraints)
        {
            solver->addHardClause(constraint, true);
        }
    }
}


void IncrementalMAFSolver::buildSolver()
{
    // Build & Initialise Solver...
    solver = std::make_unique<IncrUWrMaxSATSolver>();
    solver->initSolver(forestData_1.nodeToIndex.size());

    for(const auto& [_, index] : forestData_1.nodeToIndex) 
    {
        solver->addSoftClause(index, 1.0);
    }

    // Add Root-Constraint...
    int rootIndex = getRootIndex(*forestData_1.forestPtr);
    std::vector<int> varIndices = { rootIndex };
    solver->addHardClause(varIndices, false);
    cnt_constraints++;
    
}

bool IncrementalMAFSolver::checkMAF(std::shared_ptr<graph::Forest>& mafSolution)
{
    int n_constraints = 0;
    auto forestData_sol = buildForestData(mafSolution);

    for (const graph::Node* solutionRoot : mafSolution->Roots())
    {
        std::vector<unsigned int> labels = getSubtreeLabels(solutionRoot, mafSolution);
        std::sort(labels.begin(), labels.end());
        int subtreeConstraints = 0;
        MAX_CONSTRAINTS_PER_ROUND_PER_SUBTREE = std::max(1, static_cast<int>(MAX_CONSTRAINTS_PER_ROUND * ((double)labels.size() / MIN_CONSTRAINTS_PER_ROUND)));

        if (labels.size() < 3) continue;

        subtreeConstraints = checkTripleConstraints(subtreeConstraints, labels, ConstraintCheckType::Coverage, forestData_sol);

        if (subtreeConstraints < 0)
        {
            n_constraints += MAX_CONSTRAINTS_PER_ROUND_PER_SUBTREE;
            continue;
        }

        subtreeConstraints = checkTripleConstraints(subtreeConstraints, labels, ConstraintCheckType::All, forestData_sol); 
        if (subtreeConstraints < 0)
        {
            n_constraints += MAX_CONSTRAINTS_PER_ROUND_PER_SUBTREE;
            continue;
        }

        n_constraints += subtreeConstraints;
    }


    if (n_constraints < MIN_CONSTRAINTS_PER_ROUND)
    {   
        std::vector<LeafPair> pairs = generateLeafPairs(mafSolution);
        for (auto type : ALL_CHECK_TYPES)
        {
            n_constraints = checkPathPairConstraints(n_constraints, type, pairs, forestData_sol);
            if (n_constraints < 0 || n_constraints > MIN_CONSTRAINTS_PER_ROUND)
                return false;
            
        }
    }
    
    if (n_constraints > 0) return false;

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
    
    for (int edgeIndex : cutEdges)
    {
        assert(indexToNode.count(edgeIndex) > 0.5);
        graph::Node* child = indexToNode.at(edgeIndex);
        DeleteEdgeAction action(child, forestPtr);
        action.doAction();
    }
    return forestPtr;
}

void IncrementalMAFSolver::addInitialConstraints()
{
    int n_constraints = 0;
    std::vector<unsigned int> labels;
    for (const auto& [_, label] : (*forestData_1.forestPtr).TerminalToLabel())
    {
        labels.push_back(label);
    }
    std::sort(labels.begin(), labels.end());

    int numLeaves = labels.size();
    for (int i = 0; i + 2 < numLeaves; i += 3)
    {   
        if (checkIncompatibleTriple(labels[i], labels[i+1], labels[i+2], forestData_1, forestData_2))
        {
            generateTripleConstraint(labels[i], labels[i+1], labels[i+2]);
            n_constraints++;
        } 
    }

    for (int i = 0; i + 3 < numLeaves; i += 4)
    {   
        if (labels[i] == labels[i+1] || labels[i] == labels[i+3] || labels[i+2] == labels[i+1] || labels[i+2] == labels[i+3]) continue;
        {
            if(!areTwoPathsDisjoint(labels[i], labels[i+1], labels[i+2], labels[i+3], forestData_1)) continue;
            if( areTwoPathsDisjoint(labels[i], labels[i+1], labels[i+2], labels[i+3], forestData_2)) continue;

            generatePathPairConstraint(labels[i], labels[i+1], labels[i+2], labels[i+3]);
            n_constraints++; 
        }
    }
}

int IncrementalMAFSolver::checkTripleConstraints(int n_constraints, std::vector<unsigned int> labels, ConstraintCheckType type,
                                                const ForestData& forestData_sol)
{
    switch (type)
    {
        case ConstraintCheckType::Linear:
        {
            for (int i = 0; i + 2 < labels.size(); i += 3)
            {   
                if (checkIncompatibleTriple(labels[i], labels[i+1], labels[i+2], forestData_sol, forestData_2))
                {
                    generateTripleConstraint(labels[i], labels[i+1], labels[i+2]);
                    n_constraints++;
                    if (n_constraints >= MAX_CONSTRAINTS_PER_ROUND_PER_SUBTREE)
                        return -1;
                }
            }
            break;
        }
        case ConstraintCheckType::Coverage:
        {
            std::unordered_set<unsigned int> usedLeaves;
            int i = 0;
            int j = 1;
            int k = 2;
            while (k < labels.size())
            {
                while (i < labels.size() && usedLeaves.count(labels[i])) i++;
                while (j < labels.size() && (j <= i || usedLeaves.count(labels[j]))) j++;
                while (k < labels.size() && (k <= j || usedLeaves.count(labels[k]))) k++;
                
                if (k >= labels.size()) break;
                
                if (checkIncompatibleTriple(labels[i], labels[j], labels[k], forestData_sol, forestData_2))
                {
                    generateTripleConstraint(labels[i], labels[j], labels[k]);
                    n_constraints++;
                    if (n_constraints >= MAX_CONSTRAINTS_PER_ROUND_PER_SUBTREE)
                        return -1;
                    usedLeaves.insert(labels[i]);
                    usedLeaves.insert(labels[j]);
                    usedLeaves.insert(labels[k]);
            
                    i = k + 1; j = i + 1; k = i + 2;
                }
                else
                {
                    k++;
                }
            }
        }
        case ConstraintCheckType::All:
        {
            for(unsigned int i = 0; i < labels.size(); ++i)
            {
                for(unsigned int j = i+1; j < labels.size(); ++j)
                {
                    for(unsigned int k = j+1; k < labels.size(); ++k)
                    {
                        if (checkIncompatibleTriple(labels[i], labels[j], labels[k], forestData_sol, forestData_2))
                        {
                            generateTripleConstraint(labels[i], labels[j], labels[k]);
                            n_constraints++;
                            if (n_constraints >= MAX_CONSTRAINTS_PER_ROUND_PER_SUBTREE)
                                return -1;
                        }
                    }
                }
            }
            break;  
        }
    }
    return n_constraints;
}

int IncrementalMAFSolver::checkPathPairConstraints(int n_constraints, ConstraintCheckType type,
                                                   const std::vector<IncrementalMAFSolver::LeafPair>& pairs,
                                                   const ForestData& forestData_sol)
{
   switch (type)
   {
        case ConstraintCheckType::Linear:
        {
            for (unsigned int i = 0; i+1 < pairs.size(); i += 2)
            {
                const LeafPair& pair1 = pairs[i];
                const LeafPair& pair2 = pairs[i+1];

                if (pair1.l == pair2.l || pair1.l == pair2.r || pair1.r == pair2.l || pair1.r == pair2.r) continue;
                // but not disjoint in forest2...
                if(!areTwoPathsDisjoint(pair1.l, pair1.r, pair2.l, pair2.r, forestData_sol)) continue;
                if( areTwoPathsDisjoint(pair1.l, pair1.r, pair2.l, pair2.r, forestData_2)) continue;

                generatePathPairConstraint(pair1.l, pair1.r, pair2.l, pair2.r);
                n_constraints++;
                if (n_constraints >= MAX_CONSTRAINTS_PER_ROUND)
                    return -1;
            }
            break;
        }
        case ConstraintCheckType::Coverage:
        {
            std::unordered_set<int> usedPairs;
            int i = 0;
            int j = 1;
            
            while (j < pairs.size())
            {
                while (i < pairs.size() && usedPairs.count(i)) i++;
                while (j < pairs.size() && (j <= i || usedPairs.count(j))) j++;
                
                if (j >= pairs.size()) break;
                
                const LeafPair& pair1 = pairs[i];
                const LeafPair& pair2 = pairs[j];
                
                if (pair1.l == pair2.l || pair1.l == pair2.r || pair1.r == pair2.l || pair1.r == pair2.r)
                {
                    j++;
                    continue;
                }
                if (!areTwoPathsDisjoint(pair1.l, pair1.r, pair2.l, pair2.r, forestData_sol))
                {
                    j++;
                    continue;
                }
                if (areTwoPathsDisjoint(pair1.l, pair1.r, pair2.l, pair2.r, forestData_2))
                {
                    j++;
                    continue;
                }
                
                generatePathPairConstraint(pair1.l, pair1.r, pair2.l, pair2.r);
                n_constraints++;
                if (n_constraints >= MAX_CONSTRAINTS_PER_ROUND)
                    return -1;

                usedPairs.insert(i);
                usedPairs.insert(j);
                
                i = j + 1;
                j = i + 1;
            }
            break;
        }
        case ConstraintCheckType::All:
        {
            for (unsigned int i = 0; i < pairs.size(); ++i)
            {
                for (unsigned int j = i+1; j < pairs.size(); ++j)
                {
                    const LeafPair& pair1 = pairs[i];
                    const LeafPair& pair2 = pairs[j];
                    
                    if (pair1.l == pair2.l || pair1.l == pair2.r || pair1.r == pair2.l || pair1.r == pair2.r) continue;
                    if(!areTwoPathsDisjoint(pair1.l, pair1.r, pair2.l, pair2.r, forestData_sol)) continue;
                    if( areTwoPathsDisjoint(pair1.l, pair1.r, pair2.l, pair2.r, forestData_2)) continue;

                    generatePathPairConstraint(pair1.l, pair1.r, pair2.l, pair2.r);
                    n_constraints++;
                    if (n_constraints >= MAX_CONSTRAINTS_PER_ROUND)
                        return -1;
                }
            }
            break;
        }
    }
    return n_constraints;
}


void IncrementalMAFSolver::generateTripleConstraint(unsigned int label1, unsigned int label2, unsigned int label3)
{
    std::array<unsigned int, 3> arr = {label1, label2, label3};
    std::sort(arr.begin(), arr.end());
    std::string key = std::to_string(arr[0]) + "," + std::to_string(arr[1]) + "," + std::to_string(arr[2]);
    
    if (!addedTripleConstraints.insert(key).second)
        return;

    std::vector<int> edgesij = getPath(label1, label2, forestData_1);
    std::vector<int> edgesjk = getPath(label2, label3, forestData_1);
    std::vector<int> edgesik = getPath(label1, label3, forestData_1);

    std::vector<int> triPathEdges;
    triPathEdges.reserve(edgesij.size() + edgesjk.size() + edgesik.size());
    triPathEdges.insert(triPathEdges.end(), edgesij.begin(), edgesij.end());
    triPathEdges.insert(triPathEdges.end(), edgesjk.begin(), edgesjk.end());
    triPathEdges.insert(triPathEdges.end(), edgesik.begin(), edgesik.end());
    
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
    std::array<unsigned int, 4> arr = {lpair1, rpair1, lpair2, rpair2};
    std::sort(arr.begin(), arr.end());
    std::string key = std::to_string(arr[0]) + "," + std::to_string(arr[1]) + "," + std::to_string(arr[2]) + "," + std::to_string(arr[3]);
    
    if (!addedPathPairConstraints.insert(key).second)
        return;

    std::vector<int> edgesij = getPath(lpair1, rpair1, forestData_1);
    std::vector<int> edgespq = getPath(lpair2, rpair2, forestData_1);

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

std::vector<IncrementalMAFSolver::LeafPair> IncrementalMAFSolver::generateLeafPairs(const std::shared_ptr<graph::Forest>& mafSolution)
{
    std::vector<LeafPair> pairs;

    for (auto root : mafSolution->Roots())
    {
        std::vector<unsigned int> subtreeLabels = getSubtreeLabels(root, mafSolution);
        int n = subtreeLabels.size();

        if (n <= 1) continue;

        if (n == 2)
        {
            pairs.push_back({subtreeLabels[0], subtreeLabels[1]});
            continue;
        }

        for (int i = 0; i < n; ++i)
            for (int j = i+1; j < n; ++j)
                pairs.push_back({subtreeLabels[i], subtreeLabels[j]});
    }

    return pairs;
}

std::vector<IncrementalMAFSolver::LeafPair> IncrementalMAFSolver::generateLeafPairs(const graph::Node* rootNode, const std::shared_ptr<graph::Forest>& mafSolution)
{
    std::vector<LeafPair> pairs;
    std::vector<unsigned int> subtreeLabels = getSubtreeLabels(rootNode, mafSolution);
    int n = subtreeLabels.size();

    if (n <= 1) return pairs;

    if (n == 2)
    {
        pairs.push_back({subtreeLabels[0], subtreeLabels[1]});
        return pairs;
    }

    for (int i = 0; i < n; ++i)
        for (int j = i+1; j < n; ++j)
            pairs.push_back({subtreeLabels[i], subtreeLabels[j]});

    return pairs;
}

void recGetSubtreeLabels(const graph::Node* n, std::vector<unsigned int>& labels, const std::shared_ptr<graph::Forest>& forest)
{
    if (!n) return;
    if (!n->leftChild)
        labels.push_back(forest->TerminalToLabel().at(const_cast<graph::Node*>(n)));
    recGetSubtreeLabels(n->leftChild, labels, forest);
    recGetSubtreeLabels(n->rightChild, labels, forest);
}

std::vector<unsigned int> IncrementalMAFSolver::getSubtreeLabels(const graph::Node* subtreeRoot, const std::shared_ptr<graph::Forest>& forest)
{
    auto labels = std::vector<unsigned int>();
    recGetSubtreeLabels(subtreeRoot, labels, forest);
    return labels;

}


void IncrementalMAFSolver::setTimeOut(double time)
{
    timeOutDelay = time;
}

int IncrementalMAFSolver::getCurrentLowerBound()
{
    return currentLB;
}


}  // namespace solver