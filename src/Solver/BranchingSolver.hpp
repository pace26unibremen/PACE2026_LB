#ifndef PACE2026_BRANCHING_SOLVER_HPP
#define PACE2026_BRANCHING_SOLVER_HPP

#include "AbstractSolver.hpp"
#include "BranchingSolverConfiguration.hpp"
#include "Context.hpp"

#include <atomic>
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

    /// \brief Context information about the instance and the solver state
    std::shared_ptr<Context> context = std::make_shared<Context>();

    /// \brief Timeout flag set by an external signal handler. When true, solve() stops at the
    /// next branch rollback and returns whatever solution has been found so far. Null = disabled.
    std::atomic<bool>* timeoutFlag = nullptr;

    /// \brief The branch that leads to the solution
    std::list<std::shared_ptr<AbstractRule>> solutionBranch =
        std::list<std::shared_ptr<AbstractRule>>();

    /// \brief unapply all rules until the next branching possibility
    /// \returns Whether all rules are unapplied. (Means there is no branching possibility left)
    bool rollBackBranch();

    /// \brief Unconditionally unapplies every rule in \ref appliedRules, restoring the instance
    /// to the state it had before the current (abandoned) branch started.
    /// \note Used on timeout: \ref solutionBranch is replayed against the instance afterwards,
    /// so the instance must be back to its pristine state first, or the replay corrupts the tree.
    void unwindAppliedRules();

    /// \brief Checks whether the current instance is a better solution than the current candidate.
    /// If true, it restores all temporal changes on the instance and writes a copy in the solution field.
    /// \note Assumes that the current instance is a solution.
    void checkSolutionCandidate();

    /// \brief initializes solver specific fields in the \ref Context object of the solver.
    void initializeContext();

  public:
    /// \brief Constructor for a branching solver.
    /// \param instance to solve
    explicit BranchingSolver(const std::shared_ptr<graph::Instance>& instance);

    /// \brief Constructor for a branching solver.
    /// \param instance to solve
    /// \param configuration for the branching solver
    explicit BranchingSolver(const std::shared_ptr<graph::Instance>& instance,
                             const std::shared_ptr<solver::BranchingSolverConfiguration>& configuration);

    /// \brief Constructor for a branching solver.
    /// \param instance to solve
    /// \param configuration for the branching solver
    /// \param context additional context for the instance
    explicit BranchingSolver(const std::shared_ptr<graph::Instance>& instance,
                             const std::shared_ptr<solver::BranchingSolverConfiguration>& configuration,
                             const std::shared_ptr<solver::Context>& context);

    ~BranchingSolver() override = default;

    /// \brief starts the solver
    /// \returns true if the solver solves the instance, else false
    bool solve() override;

    /// \brief Unapplies all reduction rules, that where applied to the instance.
    void unapplyReductions() override;

    /// \brief Getter for the Context.
    const std::shared_ptr<solver::Context>& GetContext();

    /// \brief Registers a timeout flag. When the flag is set to true (e.g. from a signal handler),
    /// solve() stops at the next branch rollback and returns the best solution found so far.
    /// Pass nullptr to disable. Returns false if no solution existed when the flag fired.
    void setTimeoutFlag(std::atomic<bool>* flag);
};

}  //namespace solver

#endif  //PACE2026_BRANCHING_SOLVER_HPP
