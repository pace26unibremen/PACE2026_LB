#ifndef PACE2026_ILPMODEL_HPP
#define PACE2026_ILPMODEL_HPP

#include "../Cluster/LeastCommonAncestor.hpp"
#include "../../Graph/Forest.hpp"
#include "../../Graph/Node.hpp"

#include <vector>
#include <string>
#include <unordered_map>

namespace solver
{

// Auxiliary structure that combines all necessary Data for one forest...
struct ForestData {

        /// \brief A Forest of binary MAF-Problem.
        std::shared_ptr<graph::Forest> forestPtr;

        /// \brief Precomputed LCA table for the forest.
        /// \note Must be reinstantiated if forest changes.
        std::shared_ptr<cluster::LeastCommonAncestor> lca;

        /// \brief Precomputed Node <-> VarIndex Map for the forest.
        std::unordered_map<const graph::Node*, int> nodeToIndex;

        /// \brief Precomputed LabeltoTerminal Map for the forest.
        /// \note Must be reinstantiated if the forest changes.
        /// \note This is needed, as the reduction solver doesn't update the map.
        std::unordered_map<unsigned int, graph::Node*> labelToTerminal;

        /// \brief A Cache for computing paths between two leaves in the forest.
        mutable std::unordered_map<uint64_t, std::vector<int>> pathCache;
};

/// \brief Initialises all necessary Data for the given Forest.
/// \param forest Forest used for initialisation...
/// \return Data Object of that Forest.
[[nodiscard]]
const ForestData buildForestData(std::shared_ptr<graph::Forest> forest);

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

    /// \brief Indizes of variables included in constraint.
    std::vector<int> varIndices;

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
    /// \param varIndices List of Variables included in Constraint.
    /// \param rhsValue Right hand-side value.
    /// \param isLowerBound Direction of inequality.
    void addConstraint(std::vector<int> varIndices, double rhsValue, bool isLowerBound);
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