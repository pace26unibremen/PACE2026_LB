#ifndef PACE2026_REDUCTIONSOLVER_HPP
#define PACE2026_REDUCTIONSOLVER_HPP

#include "AbstractSolver.hpp"
#include "BranchingSolverConfiguration.hpp"
#include "Rule/SubtreeReductionRule.hpp"

namespace solver
{

class ReductionSolver : public AbstractSolver
{
private:
    std::shared_ptr<solver::AbstractRule> subtreeReductionRule = nullptr;
    std::shared_ptr<solver::Context> context = std::make_shared<solver::Context>();
public:
    /// \brief Constructor for a reduction solver.
    /// \param instance to solve
    explicit ReductionSolver(const std::shared_ptr<graph::Instance>& instance);

    /// \brief solve the instance
    bool solve() override;

    /// \brief Unapplies all reduction rules, that where applied to the instance.
    void unapplyReductions() override;

};


} // namespave solver

#endif //PACE2026_REDUCTIONSOLVER_HPP
