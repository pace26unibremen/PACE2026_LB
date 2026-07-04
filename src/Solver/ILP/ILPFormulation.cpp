#include "ILPFormulation.hpp"
#include "ILPConstraintUtils.hpp"
#include "TreeUtils.hpp"

#include <cassert>
#include <unordered_map>
#include <vector>


namespace solver {

ILPFormulation::ILPFormulation(std::shared_ptr<graph::Forest> forest1,
                                std::shared_ptr<graph::Forest> forest2,
                                unsigned int rootLabel) : 
      forestData_1(buildForestData(forest1)),
      forestData_2(buildForestData(forest2)),
      rootLabel(rootLabel)
{}

ILPProblem ILPFormulation::build() const
{
    // NOTE: Edge indices are implicitly defined by node indices in Forest::Nodes().
    // Each node i (except root) corresponds to the edge from node i to its parent.
    // Consider explicit edge indexing via unordered_map<Node*, int> for future refactoring.

    // NOTE: Assumes Forest::Roots()[0] is the root of the single tree in each forest.
    // Each forest in the instance is expected to contain exactly one tree.

    assert(forestData_1.forestPtr->Roots().size() == 1);
    assert(forestData_2.forestPtr->Roots().size() == 1);
    
    ILPProblem problem;
    auto nodeToIndex1 = buildNodeToIndexMap(*forestData_1.forestPtr);
    auto nodeToIndex2 = buildNodeToIndexMap(*forestData_2.forestPtr);
    int numVars = getNumVars(*forestData_1.forestPtr);

    if (rootLabel != 0)
    {
        int rootIndex = nodeToIndex1.at(forestData_1.labelToTerminal.at(rootLabel));
        for(int i = 0; i < numVars; i++) 
        {
            char varName[64];
            sprintf(varName, "C,%d", i);
            if (i == rootIndex)
            {
                problem.addVariable(i, varName, 0.5);
            }
            else 
            {
                problem.addVariable(i, varName); // rest of variables are default values...
            }
        }
    }
    else 
    {
        for(int i = 0; i < numVars; i++) 
        {
            char varName[64];
            sprintf(varName, "C,%d", i);
            problem.addVariable(i, varName); // rest of variables are default values...
        }

        //========= 2. Add Constraints ========

        // Root-Constraint: x_root <= 0
        int rootIndex = getRootIndex(*forestData_1.forestPtr);
        std::vector<int> varIndices = { rootIndex };
        problem.addConstraint(varIndices, 0.0, false);
    }    
    // Triple-Constraints: for every Tripel-Constraint-Set S: ∑_{i∈S} xᵢ ≥ 1
    // NOTE: A constraint here is a set of integers, that represent Variable Indices... 
    std::vector<std::vector<int>> tripleConstraints = computeTripleConstraints(forestData_1, forestData_2);
    
    for (const std::vector<int>& edges : tripleConstraints)
    {
        std::vector<int> varIndices;
        // Add each edge to varIndices...
        for (int edge : edges)
        {
            varIndices.push_back(edge);
        }
        problem.addConstraint(varIndices, 1.0, true);
    }

    // Pathpair-Constraints: for every pathpair-constraint-set S: ∑_{i∈S} xᵢ ≥ 1
    std::vector<std::vector<int>> pathPairConstraints = computePathPairConstraints(forestData_1, forestData_2);

    for (const std::vector<int>& edges : pathPairConstraints)
    {
        std::vector<int> varIndices;
        // Add each edge to varIndices...
        for (int edge : edges)
        {
            varIndices.push_back(edge);
        }
        problem.addConstraint( varIndices, 1.0, true);
    }

    // Return problem-object.
    return problem;
}

} // namespace solver