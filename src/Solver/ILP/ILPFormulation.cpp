#include "ILPFormulation.hpp"
#include "ILPConstraintUtils.hpp"
#include "TreeUtils.hpp"

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
    int rootIndex = getRootIndex(*forest1);
    std::vector<int> varIndices = { rootIndex };
    std::vector<double> coeffs = { 1.0 };
    problem.addConstraint(conNum++, varIndices, coeffs, 0.0, false);

    // Triple-Constraints: for every Tripel-Constraint-Set S: ∑_{i∈S} xᵢ ≥ 1
    // NOTE: A constraint here is a set of integers, that represent Variable Indices... 
    std::vector<std::set<int>> tripleConstraints = computeTripleConstraints(*forest1, *forest2, *lca1, *lca2, nodeToIndex1, nodeToIndex2);
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
    std::vector<std::set<int>> pathPairConstraints = computePathPairConstraints(*forest1, *forest2, *lca1, *lca2, nodeToIndex1, nodeToIndex2);
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

} // namespace solver