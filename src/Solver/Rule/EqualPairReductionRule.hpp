#ifndef PACE2026_EQUAL_PAIR_REDUCTION_RULE_HPP
#define PACE2026_EQUAL_PAIR_REDUCTION_RULE_HPP

#include "../Action/CollapseSubtreeAction.hpp"
#include "AbstractRule.hpp"

#include <stack>

namespace solver
{

/// \brief Collapses subtree of a terminal pair if it is a pair in every forest of the instance.
///
/// \see
/// <a href="https://gitlab.informatik.uni-bremen.de/pace-2026/orga/-/wikis/Branching/PairEqualRule">
/// GitLab Documentation

/// </a>
class EqualPairReductionRule : public AbstractRule
{
  protected:
    /// \brief Stack of action that modify the instance,
    /// filled in the apply method and unfilled in the unapply method
    std::stack<CollapseSubtreeAction> changes = std::stack<solver::CollapseSubtreeAction>();

    /// \brief label of one sibling nodes
    unsigned int aLabel;
    /// \brief label of the other sibling node
    unsigned int cLabel;

public:
    /// \param instance the problem instance
    /// \param context information about the instance and the solver state
    /// \param aLabel label of one sibling nodes
    /// \param cLabel label of the other sibling node
    EqualPairReductionRule(const std::shared_ptr<graph::Instance>& instance,
                  const std::shared_ptr<Context>& context,
                  unsigned int aLabel,
                  unsigned int cLabel);

    /// \brief applies rule
    /// \returns always \ref RuleReturnCode::Continue
    RuleReturnCode apply() override;

    void unapply() override;

    /// \brief It checks whether the EqualPairReductionRule is applicable and generates an instance of this rule if so.
    /// This method only considers the EqualPairReductionRule applicable if the first pair found in forest 1
    /// has a corresponding pair of terminals in each of the other forests.
    /// \param instance on which the rule should be applied
    /// \param context contains additional information to the instance and the solver state
    /// \returns shared_pointer to EqualPairReductionRule if rule is applicable, elso null pointer
    static std::shared_ptr<AbstractRule> isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                                      const std::shared_ptr<Context>& context);

    [[nodiscard]]
    std::string name() const override;

    [[nodiscard]]
    std::shared_ptr<AbstractRule> clone() const override;
};

}  //namespace solver

#endif  //PACE2026_EQUAL_PAIR_REDUCTION_RULE_HPP
