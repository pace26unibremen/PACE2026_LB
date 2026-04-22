#ifndef PACE2026_ILPMODEL_HPP
#define PACE2026_ILPMODEL_HPP

#include <vector>
#include <string>

namespace solver
{

/// \brief Represents a Variable for ILP Formulation.
struct ILPVariable
{
    /// \brief Variable number (important to match with constraints).
    int varNum;

    /// \brief Variable name.
    std::string varName;

    /// \brief Objective coefficient.
    /// \default is 1.0
    /// \note Currently all variables have objCoeff=1.0
    double objCoeff = 1.0;

    /// \brief Indicates a binary variable.
    /// \default is true
    /// \note Currently all variables are binary.
    bool isBinary = true;
};

/// \brief Represents a Constraint for ILP Formulation.
struct ILPConstraint 
{
    /// \brief Constraint number (for identification)
    int conNum;

    /// \brief Indizes of variables included in constraint.
    std::vector<int> varIndices;

    /// \brief Coefficients of each variable in constraint.
    /// \note Currently every coefficient should be 1.0.
    std::vector<double> coeffs;

    /// \brief Right-hand side value. 
    double rhsValue;

    /// \brief Indicates direction of inequality.
    /// \note true = ≥, false = ≤
    bool isLowerBound;
};

/// \brief Represents the ILP Problem Formulation.
/// Build by ILPFormulation and passed to all Solvers.
struct ILPProblem 
{
    /// \brief Indicates wether ILP should be minimized or maximized
    /// \default true (=minimize)
    bool fMinimize = true;

    /// \brief Vector of Variables.
    std::vector<ILPVariable> vars;

    /// \brief Vector of Constraints.
    std::vector<ILPConstraint> constraints;

    /// \brief Returns the number of variables in the problem.
    /// \return Number of variables.
    [[nodiscard]] 
    int nVars() const { return vars.size(); }

    /// \brief Returns the number of constraints in the problem.
    /// \return Number of constraints.
    [[nodiscard]] 
    int nConstraints() const { return constraints.size(); }   

    /// \brief Adds a Variable to the problem.
    /// \param varNum Index of Variable.
    /// \param varName Name of Variable.
    /// \param objCoeff Objective Coeffizients.
    /// \param isBinary Indicates Binary Variable.
    void addVariable(int varNum, std::string varName, double objCoeff = 1.0, bool isBinary = true);

    /// \brief Adds a Constraint to the problem.
    /// \param conNum Index of Constraint.
    /// \param varIndices List of Variables included in Constraint.
    /// \param rhsValue Right hand-side value.
    /// \param isLowerBound Direction of inequality.
    void addConstraint(int conNum, std::vector<int> varIndices, std::vector<double> coeffs, double rhsValue, bool isLowerBound);
};

/// \brief Represents the ILP Solution.
struct ILPSolution 
{
    /// \brief Optimal Value found by the solver ( SPR Distance).
    /// \default is -1.0 indicating an initial value.
    double objValue = -1.0;

    /// \brief Solution values per variable (0.0 or 1.0)
    /// \note Variable i is cut if solValues[i] > 0.5.
    std::vector<double> solValues;

    /// \brief Indicates wether the solver found a feasible solution.
    /// \default is false.
    bool feasible = false;
};

}
#endif  //PACE2026_ILPMODEL_HPP