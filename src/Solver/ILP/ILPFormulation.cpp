#include "ILPFormulation.hpp"

#include <cassert>


namespace solver {

ILPFormulation::ILPFormulation(std::shared_ptr<graph::Forest> forest1,
                                std::shared_ptr<graph::Forest> forest2): forest1(forest1), forest2(forest2),
      lca1(std::make_shared<cluster::LeastCommonAncestor>(forest1)),
      lca2(std::make_shared<cluster::LeastCommonAncestor>(forest2))
{}

ILPProblem ILPFormulation::build() const
{
    // NOTE: Edge indices are implicitly defined by node indices in Forest::Nodes().
    // Each node i (except root) corresponds to the edge from node i to its parent.
    // Consider explicit edge indexing via unordered_map<Node*, int> for future refactoring.

    // NOTE: Assumes Forest::Roots()[0] is the root of the single tree in each forest.
    // Each forest in the instance is expected to contain exactly one tree.

    assert(forest1->Roots().size() == 1);
    assert(forest2->Roots().size() == 1);
    
    ILPProblem problem;
    auto nodeToIndex1 = buildNodeToIndexMap(*forest1);
    auto nodeToIndex2 = buildNodeToIndexMap(*forest2);

    //========= 1. Add Variables ===========
    int numVars = forest1->Nodes().size();
    for(int i = 0; i < numVars; i++) 
    {
        char varName[64];
        sprintf(varName, "C,%d", i);
        problem.addVariable(i, varName); // rest of variables are default values...
    }

    //========= 2. Add Constraints ========
    int conNum = 0;

    // Root-Constraint: x_root <= 0
    int rootIndex = getRootIndex(forest1);
    std::vector<int> varIndices = { rootIndex };
    std::vector<double> coeffs = { 1.0 };
    problem.addConstraint(conNum++, varIndices, coeffs, 0.0, false);

    // Triple-Constraints: for every Tripel-Constraint-Set S: ∑_{i∈S} xᵢ ≥ 1
    // NOTE: A constraint here is a set of integers, that represent Variable Indices... 
    std::vector<std::set<int>> tripleConstraints = computeTripleConstraints(nodeToIndex1, nodeToIndex2);
    for (const std::set<int>& edges : tripleConstraints)
    {
        std::vector<int> varIndices;
        std::vector<double> coeffs;
        // Add each edge to varIndices and coeffs...
        for (int edge : edges)
        {
            varIndices.push_back(edge);
            coeffs.push_back(1.0);
        }
        problem.addConstraint(conNum++, varIndices, coeffs, 1.0, true);
    }

    // Pathpair-Constraints: for every pathpair-constraint-set S: ∑_{i∈S} xᵢ ≥ 1
    std::vector<std::set<int>> pathPairConstraints = computePathPairConstraints(nodeToIndex1, nodeToIndex2);
    for (const std::set<int>& edges : pathPairConstraints)
    {
        std::vector<int> varIndices;
        std::vector<double> coeffs;
        // Add each edge to varIndices and coeffs...
        for (int edge : edges)
        {
            varIndices.push_back(edge);
            coeffs.push_back(1.0);
        }
        problem.addConstraint(conNum++, varIndices, coeffs, 1.0, true);
    }

    // Return problem-object.
    return problem;
}

std::vector<std::set<int>> ILPFormulation::computeTripleConstraints(
                        const std::unordered_map<const graph::Node*, int>& nodeToIndex1,
                        const std::unordered_map<const graph::Node*, int>& nodeToIndex2) const
{
    // Both Forests should have the same (number) of leaves...
    assert(forest1->LabelToTerminal().size() == forest2->LabelToTerminal().size());

    // Both Index-Maps should have the same size as the Nodes()...
    assert(nodeToIndex1.size() == forest1->Nodes().size());
    assert(nodeToIndex2.size() == forest2->Nodes().size());

    std::vector<std::set<int>> tripleConstraints;
    int numLeaves = forest1->LabelToTerminal().size();

    for(unsigned int i = 1; i < numLeaves; ++i)
    {
        for(unsigned int j = i+1; j < numLeaves; ++j)
        {
            for(unsigned int k = j+1; i < numLeaves; ++k)
            {
                bool fIncTriple = checkIncompatibleTriple(i, j, k, nodeToIndex1, nodeToIndex2);
                
                // If Triple is compatible just continue...
                if (!fIncTriple) continue;

                std::set<int> edgesij = getPath(*forest1, i, j, *lca1, nodeToIndex1);
                std::set<int> edgesjk = getPath(*forest1, j, k, *lca1, nodeToIndex1);
                std::set<int> edgesik = getPath(*forest1, i, k, *lca1, nodeToIndex1);

                // Union aller drei Pfade
                std::set<int> triPathEdges = edgesij;
                triPathEdges.insert(edgesjk.begin(), edgesjk.end());
                triPathEdges.insert(edgesik.begin(), edgesik.end());

                tripleConstraints.push_back(triPathEdges);

            }
        }
    }
}

std::vector<std::set<int>> ILPFormulation::computePathPairConstraints(
                        const std::unordered_map<const graph::Node*, int>& nodeToIndex1,
                        const std::unordered_map<const graph::Node*, int>& nodeToIndex2) const
{
    // Both Forests should have the same (number) of leaves...
    assert(forest1->LabelToTerminal().size() == forest2->LabelToTerminal().size());

    // Both Index-Maps should have the same size as the Nodes()...
    assert(nodeToIndex1.size() == forest1->Nodes().size());
    assert(nodeToIndex2.size() == forest2->Nodes().size());

    std::vector<std::set<int>> pathPairConstraints;
    int numLeaves = forest1->LabelToTerminal().size();

    for(unsigned int i = 1; i < numLeaves; ++i)
    {
        for(unsigned int j = i+1; j < numLeaves; ++j)
        {
            for(unsigned int p = i+1; p < numLeaves; ++p)
            {
                // Only look at unique leaf pairs...
                if(p == j) continue;
                for(unsigned int q = p+1; q < numLeaves; ++q)
                {
                    // Only look at unique leaf pairs...
                    if(q == j) continue;

                    // Only Important case is, if paths of pairs are disjoint in forest1
                    // but not disjoint in forest2...
                    if(!areTwoPathsDisjoint(*forest1, i, j, p, q, *lca1, nodeToIndex1)) continue;
                    if( areTwoPathsDisjoint(*forest2, i, j, p, q, *lca2, nodeToIndex2)) continue;

                    std::set<int> edgesij = getPath(*forest1, i, j, *lca1, nodeToIndex1);
                    std::set<int> edgespq = getPath(*forest1, p, q, *lca1, nodeToIndex1);

                    std::set<int> pathPairEdges = edgesij;
                    pathPairEdges.insert(edgespq.begin(), edgespq.end());

                    pathPairConstraints.push_back(pathPairEdges);
                }
            }
        }
    }

    return pathPairConstraints;
}

bool ILPFormulation::checkIncompatibleTriple(unsigned int leaf1, unsigned int leaf2, unsigned int leaf3, 
                                const std::unordered_map<const graph::Node*, int>& nodeToIndex1,
                                const std::unordered_map<const graph::Node*, int>& nodeToIndex2) const
{
    // Leaves should be different from each other...
    assert(leaf1 != leaf2);
    assert(leaf2 != leaf3);
    assert(leaf1 != leaf3);

    // Labels should exist in both forests...
    assert(forest1->LabelToTerminal().count(leaf1) > 0);
    assert(forest1->LabelToTerminal().count(leaf2) > 0);
    assert(forest1->LabelToTerminal().count(leaf3) > 0);
    assert(forest2->LabelToTerminal().count(leaf1) > 0);
    assert(forest2->LabelToTerminal().count(leaf2) > 0);
    assert(forest2->LabelToTerminal().count(leaf3) > 0);

    bool fIncTriple = false;

    int lcaij1 = getLCA(*forest1, *lca1, leaf1, leaf2, nodeToIndex1);
    int lcajk1 = getLCA(*forest1, *lca1, leaf2, leaf3, nodeToIndex1);
    int lcaik1 = getLCA(*forest1, *lca1, leaf1, leaf3, nodeToIndex1);
    int lcaij2 = getLCA(*forest2, *lca2, leaf1, leaf2, nodeToIndex2);
    int lcajk2 = getLCA(*forest2, *lca2, leaf2, leaf3, nodeToIndex2);
    int lcaik2 = getLCA(*forest2, *lca2, leaf1, leaf3, nodeToIndex2);

    if(lcaij1 == lcajk1)
    {
        if(lcaij2 != lcajk2) fIncTriple = true;
    }
    else if(lcaij1 == lcaik1)
    {
        if(lcaij2 != lcaik2) fIncTriple = true;
    }
    else
    {
        if(lcaik2 != lcajk2) fIncTriple = true;
    }

    return fIncTriple;
}

} // namespace solver