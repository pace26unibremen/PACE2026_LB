#ifndef PACE2026_INCRUWRMAXSATSOLVER_HPP
#define PACE2026_INCRUWRMAXSATSOLVER_HPP

#include "../AbstractIncrementalSolver.hpp"
#include "../ILPModel.hpp"

namespace solver {

class IncrUWrMaxSatSolver : public AbstractIncrementalSolver
{
public:
    IncrUWrMaxSatSolver() = default;
    ~IncrUWrMaxSatSolver() override;

    void initSolver() override;
    void addSoftClause(int id, double objCoeff) override;
    void addHardClause(const std::vector<int>& varIDs, bool isLowerBound) override;
    ILPSolution solve(int numVars) override;
    int getVarVal(int var) const override;
    double getCost() const override;

private:
    void* solver_ipamir = nullptr;

}; // class IncrUWrMaxSatSolver

} // namespace solver

#endif // PACE2026_INCRUWRMAXSATSOLVER_HPP