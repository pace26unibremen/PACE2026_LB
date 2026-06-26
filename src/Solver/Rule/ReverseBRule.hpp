#ifndef PACE2026_REVERSE_B_RULE_HPP
#define PACE2026_REVERSE_B_RULE_HPP

#include "../Action/DeleteEdgeAction.hpp"
#include "AbstractRule.hpp"

#include <stack>

namespace solver
{

/// \brief This rule does the reverse of rule B,
/// cutting out the middle node without the need to branch.
class ReverseBRule : public  AbstractRule
{
  protected:

    unsigned int toCutLabel;

    /// \brief Stack of action that modify the instance,
    /// filled in the apply method and unfilled in the unapply method
    std::stack<DeleteEdgeAction> changes = std::stack<DeleteEdgeAction>();

public:
    ReverseBRule(const std::shared_ptr<graph::Instance>& instance,
                  const std::shared_ptr<Context>& context,
                  const unsigned int& toCutLabel);

    /// \brief applies rule
    /// \returns some RuleReturnCode
    RuleReturnCode apply() override;

    void unapply() override;

    /// \brief It checks whether the ReverseCaseBRule is applicable and generates an instance of this rule if so.
    /// \param instance on which the rule should be applied
    /// \param context contains additional information to the instance and the solver state
    /// \returns shared_pointer to ReverseCaseBRule if rule is applicable, else null pointer
    static std::shared_ptr<AbstractRule> isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                                      const std::shared_ptr<Context>& context);

    [[nodiscard]]
    std::string name() const override;

    [[nodiscard]]
    std::shared_ptr<AbstractRule> clone() const override;
};

}  //namespace solver

#endif  //PACE2026_REVERSE_B_RULE_HPP
