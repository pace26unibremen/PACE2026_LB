#ifndef PACE2026_SUBTREE_REDUCTION_RULE_HPP
#define PACE2026_SUBTREE_REDUCTION_RULE_HPP

#include "../Action/CollapseSubtreeAction.hpp"
#include "AbstractRule.hpp"

#include <stack>

namespace solver
{

/// \brief Collapses all subtrees that are identical in every forest of the instance.
///
/// \see similar to \ref PairEqualRule, which just collapses simple subtrees. See therefore
/// <a href="https://gitlab.informatik.uni-bremen.de/pace-2026/orga/-/wikis/Branching/EqualForestsRule">
/// GitLab Documentation
/// </a>
class SubtreeReductionRule : public AbstractRule
{
  protected:
    /// \brief A mapping of each forest to a list of subtrees that can be collapsed
    std::unordered_map<std::shared_ptr<graph::Forest>, std::list<graph::Node*>> forestToSubtrees;

    /// \brief Stack of actions that modify the instance,
    /// filled in the apply method and unfilled in the unapply method
    std::stack<CollapseSubtreeAction> changes = std::stack<solver::CollapseSubtreeAction>();

  public:
    /// \param instance the problem instance
    /// \param context information about the instance and the solver state
    /// \param forestToSubtrees the position where the rule can be applied,\n
    /// which is a mapping of each forest to a list of subtrees (via the subtree root) in this forest
    /// that can be collapsed
    SubtreeReductionRule(
        const std::shared_ptr<graph::Instance>& instance, const std::shared_ptr<Context>& context,
        const std::unordered_map<std::shared_ptr<graph::Forest>, std::list<graph::Node*>>& forestToSubtrees);

    /// \brief applies rule
    /// \returns always \ref RuleReturnCode::Continue
    RuleReturnCode apply() override;

    void unapply() override;

    /// \brief It checks whether the SubtreeReductionRule is applicable and generates an instance of this rule if so.
    /// This method only considers the SubtreeReductionRule applicable if there is any subtree that is identical
    /// in every forest. The constructed rule collapses all identical subtrees.
    /// \param instance on which the rule should be applied
    /// \param context contains additional information to the instance and the solver state
    /// \returns shared_pointer to SubtreeReductionRule if rule is applicable, else null pointer
    /// \note This method assumes that all forests of the instance were subject to the same collapsing actions.
    static std::shared_ptr<AbstractRule> isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                                      const std::shared_ptr<Context>& context);

    [[nodiscard]]
    std::string name() const override;

    [[nodiscard]]
    std::shared_ptr<AbstractRule> clone() const override;
};

}  //namespace solver

#endif  //PACE2026_SUBTREE_REDUCTION_RULE_HPP
