#ifndef PACE2026_REDUCTIONSOLVER_HPP
#define PACE2026_REDUCTIONSOLVER_HPP

#include "AbstractSolver.hpp"
#include "BranchingSolverConfiguration.hpp"

#include <list>

namespace solver
{

class ReductionSolver : public AbstractSolver
{
private:
    std::list<std::shared_ptr<solver::AbstractRule>> appliedRules;
    std::shared_ptr<solver::Context> context = std::make_shared<solver::Context>();
public:
    /// \brief Constructor for a reduction solver.
    /// \param instance to solve
    explicit ReductionSolver(const std::shared_ptr<graph::Instance>& instance);

    /// \brief solve the instance: applies SubtreeReductionRule and ChainReductionRule
    /// repeatedly until neither is applicable anymore (a true fixpoint).
    bool solve() override;

    /// \brief Unapplies all reduction rules that were applied, in reverse order.
    void unapplyReductions() override;

};


} // namespave solver

#endif //PACE2026_REDUCTIONSOLVER_HPP
