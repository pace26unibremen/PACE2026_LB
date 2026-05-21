#include "ILPModel.hpp"

void solver::ILPProblem::addVariable(int varNum, std::string varName, 
                            double objCoeff, bool isBinary)
{
    ILPVariable var = {varNum, std::move(varName), objCoeff, isBinary};
    vars.push_back(var);
}

void solver::ILPProblem::addConstraint(std::vector<int> varIndices, double rhsValue, bool isLowerBoundCon)
{
        constraintStart.push_back(allVarIndices.size());
        allVarIndices.insert(allVarIndices.end(), varIndices.begin(), varIndices.end());
        rhsValues.push_back(rhsValue);
        isLowerBound.push_back(isLowerBoundCon);
}

