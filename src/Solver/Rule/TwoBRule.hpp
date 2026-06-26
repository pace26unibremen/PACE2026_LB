#ifndef PACE2026_TWO_B_RULE_HPP
#define PACE2026_TWO_B_RULE_HPP

#include "../Action/DeleteEdgeAction.hpp"
#include "AbstractRule.hpp"

#include <stack>

namespace solver
{

/// \brief This rule implements Whidden's 2B rule,
/// cutting out two nodes if the corresponding structure is found.
class TwoBRule : public  AbstractRule
{
  protected:

    unsigned int toCutLabel1;

    unsigned int toCutLabel2;

    /// \brief Stack of action that modify the instance,
    /// filled in the apply method and unfilled in the unapply method
    std::stack<DeleteEdgeAction> changes = std::stack<DeleteEdgeAction>();

public:
    TwoBRule(const std::shared_ptr<graph::Instance>& instance,
                  const std::shared_ptr<Context>& context,
                  const std::pair<unsigned int, unsigned int>& toCutLabelPair);

    /// \brief applies rule
    /// \returns some RuleReturnCode
    RuleReturnCode apply() override;

    void unapply() override;

    /// \brief It checks whether the TwoBRule is applicable and generates an instance of this rule if so.
    /// \param instance on which the rule should be applied
    /// \param context contains additional information to the instance and the solver state
    /// \returns shared_pointer to TwoBRule if rule is applicable, else null pointer
    static std::shared_ptr<AbstractRule> isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                                      const std::shared_ptr<Context>& context);

    [[nodiscard]]
    std::string name() const override;

    [[nodiscard]]
    std::shared_ptr<AbstractRule> clone() const override;
};

}  //namespace solver

#endif  //PACE2026_TWO_B_RULE_HPP
