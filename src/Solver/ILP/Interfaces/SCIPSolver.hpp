#ifndef PACE2026_SCIPSOLVER_HPP
#define PACE2026_SCIPSOLVER_HPP

#include "../AbstractILPSolver.hpp"
#include "../ILPModel.hpp"

#include <scip/scip.h>
#include <scip/scipdefplugins.h>
#include "scip/cons_linear.h"
#include "scip/def.h"

#include <vector>
#include <memory>

namespace solver {

// Custom Deleter für SCIP
struct SCIPDeleter {
    void operator()(SCIP* s) const {
        if (s) SCIPfree(&s);
    }
};

/// \brief Concrete ILP solver backend using SCIP.
/// Solves ILPProblem via SCIP ILP Solver
class SCIPSolver : public AbstractILPSolver
{
public:
    SCIPSolver();
    ~SCIPSolver() override = default;

    /// \brief Solves the ILP problem using SCIP Solver.
    /// \note Creates SCIP instance, starts Solver and writes solution.
    /// \param problem The ILP problem to solve.
    /// \returns The ILP solution. solution.feasible is false if no solution was found.
    [[nodiscard]]
    ILPSolution solve(const ILPProblem problem) override;

private:
    /// \brief Vector of SCIP Variables.
    std::vector<SCIP_VAR*> vars;
    /// \brief Vector of SCIP Constraints.
    std::vector<SCIP_CONS*> constraints;
    /// \brief SCIP Environment.
    std::unique_ptr<SCIP, SCIPDeleter> scip;

    /// \brief Initiates the SCIP Solver Environment.
    void init();

    /// \brief Initiates & Adds Problem Instance to SCIP Environment.
    /// \param nCols Number of Columns in the problem (equiv to number of variables).
    /// \param nRows Number of Rows in the problem (equiv to number of constraints).
    /// \param fMinimize Indicates wether ILP should be minimized or maximized.
    void createProblem(int nCols, int nRows, bool fMinimize);

    /// \brief Sets up a variable in the SCIP solver.
    /// \note All Variables should be binary and have an objCoeff of 1.0.
    /// \param varNum Variable number.
    /// \param varName Variable name.
    /// \param objCoeff Objective coefficient.
    /// \param isBinary Indicates a binary variable.
    void setupVar(int varNum, const char* varName, double objCoeff, bool isBinary);

    /// \brief Sets up a constraint in the SCIP solver.
    /// \note Every variable coefficient of every constraint should be 1.0.
    /// \param conNum Constraint number (for identification)
    /// \param varIndices Indizes of (non-zero) variables included in constraint.
    /// \param coeffs Coefficients of each variable in constraint.
    /// \param rhsValue Right-hand side value.
    /// \param isLowerBound Indicates direction of inequality (true = ≥, false = ≤).
    void addConstraint(int conNum, const std::vector<int> &varIndices, const std::vector<double> &coeffs, 
                        double rhsValue,  bool isLowerBound);

}; // class SCIP

} // namespace solver

#endif // PACE2026_SCIPSOLVER_HPP