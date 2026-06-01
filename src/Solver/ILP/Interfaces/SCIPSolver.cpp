#include "SCIPSolver.hpp"

#include <cassert>
#include <iostream>

namespace solver {

// Constructor sets up SCIP environment...
SCIPSolver::SCIPSolver() : scip(nullptr)
{
    init();
}

ILPSolution SCIPSolver::solve(const ILPProblem problem)
{
    // Make sure it is a minimization problem...
    assert(problem.fMinimize);

    ILPSolution sol;

    // Setup Problem...
    createProblem(problem.nVars(), problem.nConstraints(), problem.fMinimize);
    // Setup Variables...
    for (const ILPVariable& var : problem.vars)
    {   
        // Make sure it is a binary variable...
        assert(var.isBinary);
        // Make sure objecitve coefficients are correkt for MaxSAT...
        assert(var.objCoeff == 1.0);

        setupVar(var.varNum, var.varName.c_str(), var.objCoeff, var.isBinary);
    }
    // Add Constraints...
    for (int i = 0; i < problem.constraints.size(); i++)
    {   
        const ILPConstraint& constraint = problem.constraints[i];
        std::vector<double> coeffs(constraint.varIndices.size(), 1.0);
    
        addConstraint(i, constraint.varIndices, coeffs, constraint.rhsValue, constraint.isLowerBound);
    }

    // Solve the Problem...

    // Start Solve...
    SCIPsolve(scip.get());

    // Retrieve Solution...
    SCIP_SOL* bestSol = SCIPgetBestSol(scip.get());
    if (bestSol == nullptr) 
    {
        return sol; // by default solution is not feasible... 
    } 


    sol.feasible = true;
    sol.objValue = static_cast<double>(SCIPgetSolOrigObj(scip.get(), bestSol));
    sol.solValues.resize(problem.nVars(), 0.0);
    for(int i = 0; i < problem.nVars(); ++i)
        sol.solValues[i] = SCIPgetSolVal(scip.get(), bestSol, vars[i]);

    return sol;
}

// Sets up SCIP Environment...
void SCIPSolver::init()
{
    SCIP* raw = nullptr;
    SCIPcreate(&raw);
    SCIPincludeDefaultPlugins(raw);
    SCIPmessagehdlrSetQuiet(SCIPgetMessagehdlr(raw), TRUE);
    scip.reset(raw);
}

// Sets up SCIP Problem...
void SCIPSolver::createProblem(int nCols, int nRows, bool fMinimize)
{
    // If there already exists a problem, reset it...
    if(SCIPgetStage(scip.get()) >= SCIP_STAGE_PROBLEM )
    {
            SCIPfreeTransform(scip.get());
            SCIPfreeProb(scip.get());
    }

    SCIPcreateProbBasic(scip.get(), "ILP_Problem");
    SCIPsetObjsense(scip.get(),fMinimize ? SCIP_OBJSENSE_MINIMIZE : SCIP_OBJSENSE_MAXIMIZE);

    vars.clear();
    constraints.clear();
    vars.resize(nCols, nullptr);
    constraints.resize(nRows, nullptr);
}

void SCIPSolver::setupVar(int varNum, const char* varName, double objCoeff, bool isBinary)
{
    // Make sure that var index is in bounds...
    assert(varNum < vars.size());

    // Set up SCIP-Variable...
    SCIP_VAR* var;
    SCIP_VARTYPE vartype = isBinary ? SCIP_VARTYPE_BINARY : SCIP_VARTYPE_INTEGER;
    SCIP_Real lb = 0.0;
    SCIP_Real ub = isBinary ? 1.0 : SCIPinfinity(scip.get());

    SCIPcreateVarBasic(scip.get(), &var, varName, lb, ub, objCoeff, vartype);
    SCIPaddVar(scip.get(), var);

    vars[varNum] = var;
    SCIPreleaseVar(scip.get(), &var); // necessary for correct reference count...
}

void SCIPSolver::addConstraint(int conNum, const std::vector<int> &varIndices, const std::vector<double> &coeffs, 
                        double rhsValue,  bool isLowerBound)
{
    // Make sure that constraint index is in bounds...
    assert(conNum < constraints.size());

    // Setup SCIP-Constraint...
    SCIP_CONS* cons;
    SCIP_Real lhs = isLowerBound ? rhsValue : -SCIPinfinity(scip.get());
    SCIP_Real rhs = isLowerBound ? SCIPinfinity(scip.get()) : rhsValue;

    SCIPcreateConsLinear(scip.get(), &cons, ("row_" + std::to_string(conNum)).c_str(), 0, nullptr, nullptr, lhs, rhs, 
            TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE);
    
    for(size_t i = 0; i < varIndices.size(); ++i)
    {
            SCIPaddCoefLinear(scip.get(), cons, vars[varIndices[i]], coeffs[i]); // Terms and Coefficients
    }

    SCIPaddCons(scip.get(), cons);

    constraints[conNum] = cons;
    SCIPreleaseCons(scip.get(), &cons); // necessary for correct reference count...
}

} // namespace solver