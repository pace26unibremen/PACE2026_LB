#ifndef PACE2026_BRANCHING_SOLVER_HPP
#define PACE2026_BRANCHING_SOLVER_HPP

#include "AbstractSolver.hpp"
#include "BranchingSolverConfiguration.hpp"
#include "Context.hpp"

#include <queue>

namespace solver
{

/// \brief The Branching Solver solves the MAF problem by repeatedly applying \ref AbstractRule "rules",
/// including \ref AbstractBranchingRule "branching rules", so that the solver's search space is a tree.
class BranchingSolver : public AbstractSolver
{
  protected:
    /// \brief The configuration of the branching solver.
    const std::shared_ptr<BranchingSolverConfiguration>
    configuration = std::make_shared<BranchingSolverConfiguration>();

    /// \brief stores all applied rules of the current branch in the order in which they were applied.
    std::list<std::shared_ptr<AbstractRule>> appliedRules = std::list<std::shared_ptr<AbstractRule>>();

    /// \brief queue of rules, that should be applied next
    std::queue<std::shared_ptr<AbstractRule>> applyNext = std::queue<std::shared_ptr<AbstractRule>>();

    /// \brief Stores the best solution, that the solver found so far.
    std::shared_ptr<graph::Forest> solution = nullptr;

    /// \brief Context information about the instance and the solver state
    std::shared_ptr<Context> context = std::make_shared<Context>();

    /// \brief unapply all rules until the next branching possibility
    /// \returns Whether all rules are unapplied. (Means there is no branching possibility left)
    bool rollBackBranch();

    /// \brief Checks whether the current instance is a better solution than the current candidate.
    /// If true, it restores all temporal changes on the instance and writes a copy in the solution field.
    /// \note Assumes that the current instance is a solution.
    void checkSolutionCandidate();

  public:
    /// \brief Constructor for a branching solver.
    /// \param instance to solve
    explicit BranchingSolver(const std::shared_ptr<graph::Instance>& instance);

    /// \brief Constructor for a branching solver.
    /// \param instance to solve
    /// \param configuration for the branching solver
    explicit BranchingSolver(const std::shared_ptr<graph::Instance>& instance,
                             const std::shared_ptr<solver::BranchingSolverConfiguration>& configuration);

    ~BranchingSolver() override = default;

    /// \brief starts the solver
    /// \returns true if the solver solves the instance, else false
    bool solve() override;

    /// \brief Unapplies all reduction rules, that where applied to the instance.
    void unapplyReductions() override;
};

}  //namespace solver

#endif  //PACE2026_BRANCHING_SOLVER_HPP
