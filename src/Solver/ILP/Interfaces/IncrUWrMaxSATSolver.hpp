#ifndef PACE2026_INCRUWRMAXSATSOLVER_HPP
#define PACE2026_INCRUWRMAXSATSOLVER_HPP

#include "../AbstractIncrementalSolver.hpp"
#include "../ILPModel.hpp"
#include "chrono"

namespace solver {

class IncrUWrMaxSATSolver : public AbstractIncrementalSolver
{
public:


    IncrUWrMaxSATSolver() = default;
    ~IncrUWrMaxSATSolver() override;

    void initSolver(int numVars) override;
    void addSoftClause(int id, double objCoeff) override;
    void addHardClause(const std::vector<int>& varIndices, bool isLowerBound) override;
    void addAssumptions(const std::vector<double>& solValues) override;
    ILPSolution solve(int numVars) override;

    void stampSolver(double seconds);


private:
    std::chrono::steady_clock::time_point future;


    void* solver_ipamir = nullptr;

}; // class IncrUWrMaxSatSolver

} // namespace solver

#endif // PACE2026_INCRUWRMAXSATSOLVER_HPP