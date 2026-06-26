#ifndef PACE2026_ABSTRACT_PLUGIN_HPP
#define PACE2026_ABSTRACT_PLUGIN_HPP

#include "../Rule/AbstractRule.hpp"

#include <memory>

namespace solver::plugin
{

/// \brief Abstract base class for branching solver plugins.
///
/// Plugins observe solver events through a set of virtual hooks. All hooks
/// have default no-op implementations so subclasses only override what they need.
///
/// The solver holds a list of plugins and invokes each hook on every plugin
/// after the corresponding solver event occurs.
///
/// The base class stores the \ref instance_ and \ref context_ pointers after
/// \ref init() is called, so subclasses can access them without re-declaring
/// those fields.
class AbstractPlugin
{
  protected:
    /// \brief The problem instance being solved. Set by \ref init().
    std::shared_ptr<graph::Instance> instance_;

    /// \brief Shared solver context (bestSolutionSize, maxSolutionSize, …). Set by \ref init().
    std::shared_ptr<solver::Context> context_;

  public:
    virtual ~AbstractPlugin() = default;

    /// \brief Called once before solving starts. Stores \p instance and \p context
    ///        in \ref instance_ and \ref context_ for use by subclasses.
    /// \param instance the problem instance that will be solved
    /// \param context  shared solver context
    virtual void init(const std::shared_ptr<graph::Instance>& instance,
                      const std::shared_ptr<solver::Context>& context);

    /// \brief Called immediately before each rule application.
    virtual void beforeApply(const std::shared_ptr<solver::AbstractRule>& rule);

    /// \brief Called after each rule application.
    virtual void onApply(const std::shared_ptr<solver::AbstractRule>& rule);

    /// \brief Called after each rule unapplication during branch rollback.
    virtual void onUnapply(const std::shared_ptr<solver::AbstractRule>& rule);

    /// \brief Called when a reduction rule is unapplied to expose a solution candidate.
    /// \param lastRule true if this is the last reduction being unapplied in this pass
    virtual void onReductionUnapply(const std::shared_ptr<solver::AbstractRule>& rule, bool lastRule);

    /// \brief Called when a reduction rule is re-applied after a solution candidate was recorded.
    virtual void onReductionReapply(const std::shared_ptr<solver::AbstractRule>& rule);

    /// \brief Called when a new best solution is found.
    /// \param weight of the new best solution
    virtual void onNewBestSolution(float weight);

    /// \brief Called when the current search branch reaches a terminal state.
    ///
    /// Fired on \c CutBranch (branch pruned) and \c EndBranchWithSolutionCandidate
    /// (solution reached), in both bounded and unbounded depth-search modes.
    /// Not fired on \c ImidateReturn (abort).
    virtual void onBranchEnd();

    /// \brief Called once when solving is complete.
    ///
    /// Guaranteed to be called after at least one \ref onBranchEnd in normal
    /// operation. The only exception is \c ImidateReturn (solver abort), where
    /// \ref onBranchEnd is intentionally skipped and \c onEnd may fire without
    /// a preceding \ref onBranchEnd for the current branch.
    virtual void onEnd();
};

} // namespace solver::plugin

#endif  //PACE2026_ABSTRACT_PLUGIN_HPP
