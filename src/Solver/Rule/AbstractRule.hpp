#ifndef PACE2026_ABSTRACT_RULE_HPP
#define PACE2026_ABSTRACT_RULE_HPP

#include "../../Graph/Instance.hpp"
#include "../Context.hpp"
#include "RuleReturnCode.hpp"

#include <list>

namespace solver
{

/// \brief A rule changes a problem instance
/// (generally without changing the solution(-size)).
/// An instance of a rule holds information about the specific part of the problem instance that can be changed.
class AbstractRule
{
  protected:
    /// \brief The problem instance to be modified
    std::shared_ptr<graph::Instance> instance;

    /// \brief Context information about the instance and the solver state
    std::shared_ptr<Context> context;

    /// \brief Stores if the rule is already applied
    bool isApplied = false;

    /// \brief It indicates if the rule is a reduction, which is a rule that must be rolled back
    /// to transform the found solution to the solution of the original instance.
    const bool isReduction;

  public:
    /// constructor
    AbstractRule(const std::shared_ptr<graph::Instance>&,
                 const std::shared_ptr<Context>& context,
                 bool isReduction);

    /// destructor
    virtual ~AbstractRule() = default;

    /// \brief applies rule
    /// \returns a return code, see \ref RuleReturnCode
    virtual RuleReturnCode apply() = 0;

    /// \brief reverts the changes of the rule
    virtual void unapply() = 0;

    /// \brief returns true if the rule is applied on the instance
    [[nodiscard, maybe_unused]]
    bool IsApplied() const;

    /// \brief returns if the rule is a reduction.
    /// \ref isReduction
    [[nodiscard]]
    bool IsReduction() const;

    /// \brief name of the rule
    [[nodiscard]]
    virtual std::string name() const = 0;

    /// \brief A list of rules that should be applied next.
    ///
    /// If the \ref apply method returns \ref RuleReturnCode::ContinueWithRuleSuggestion
    /// this indicates that the list contains items and the solver should adopt them.
    ///
    /// The first element of the list should be applied first, followed by the next element, and so on.
    virtual std::shared_ptr<std::list<std::shared_ptr<AbstractRule>>> NextRuleSuggestion();

    /// Creates a clone of the rule, i.e. a rule that will be applied to the same instance location.
    /// \returns the clone
    /// \warning This is not a copy, as it does not take into account whether the rule has been applied, etc.
    [[nodiscard]]
    virtual std::shared_ptr<AbstractRule> clone() const = 0;
};

} // namespace solver

#endif  //PACE2026_ABSTRACT_RULE_HPP
