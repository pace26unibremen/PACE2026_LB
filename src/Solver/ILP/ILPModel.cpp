#include "ILPModel.hpp"

void solver::ILPProblem::addVariable(int varNum, std::string varName, 
                            double objCoeff, bool isBinary)
{
    ILPVariable var = {varNum, std::move(varName), objCoeff, isBinary};
    vars.push_back(var);
}

void solver::ILPProblem::addConstraint(std::vector<int> varIndices, double rhsValue, bool isLowerBound)
{
    ILPConstraint constraint = {varIndices, rhsValue, isLowerBound};
    constraints.push_back(constraint);
}

