#include "ILPModel.hpp"
#include "../Cluster/LeastCommonAncestor.hpp"
#include "../../Graph/Forest.hpp"
#include "TreeUtils.hpp"

namespace solver {

void ILPProblem::addVariable(int varNum, std::string varName, 
                            double objCoeff, bool isBinary)
{
    ILPVariable var = {varNum, std::move(varName), objCoeff, isBinary};
    vars.push_back(var);
}

void ILPProblem::addConstraint(std::vector<int> varIndices, double rhsValue, bool isLowerBound)
{
    ILPConstraint constraint = {varIndices, rhsValue, isLowerBound};
    constraints.push_back(constraint);
}

const ForestData buildForestData(std::shared_ptr<graph::Forest> forest)
{
    return {
        forest,
        std::make_shared<cluster::LeastCommonAncestor>(forest),
        buildNodeToIndexMap(*forest),
        buildLabelToTerminal(*forest)
    };
}

} // namespace solver

