#ifndef PACE2026_INCREVALMAXSATSOLVER_HPP
#define PACE2026_INCREVALMAXSATSOLVER_HPP

#include "../AbstractIncrementalSolver.hpp"
#include "../ILPModel.hpp"
#include "EvalMaxSAT.h"
#include <vector>
#include <memory>

namespace solver {

class IncrEvalMaxSATSolver : public AbstractIncrementalSolver
{
public:
    IncrEvalMaxSATSolver() = default;
    ~IncrEvalMaxSATSolver() override = default;

    void initSolver(int numVars) override;
    void addSoftClause(int id, double objCoeff) override;
    void addHardClause(const std::vector<int>& varIndices, bool isLowerBound) override;
    void addAssumptions(const std::vector<double>& solValues) override;
    ILPSolution solve(int numVars) override;

private:
    EvalMaxSAT<> solver_evalmaxsat;
    std::unordered_map<int, int> varIDs;

}; // class IncrEvalMaxSatSolver

} // namespace solver

#endif // PACE2026_INCREVALMAXSATSOLVER_HPP