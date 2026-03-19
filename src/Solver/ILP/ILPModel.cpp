#include "ILPModel.hpp"

void solver::ILPProblem::addVariable(int varNum, std::string varName, 
                            double objCoeff, bool isBinary)
{
    ILPVariable var = {varNum, std::move(varName), objCoeff, isBinary};
    vars.push_back(var);
}

void solver::ILPProblem::addConstraint(int conNum, std::vector<int> varIndices, 
                                std::vector<double> coeffs, double rhsValue, bool isLowerBound)
{
    ILPConstraint constraint = {conNum, varIndices, coeffs, rhsValue, isLowerBound};
    constraints.push_back(constraint);
}

