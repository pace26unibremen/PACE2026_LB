#ifndef PACE2026_SINGLE_VERTEX_TREE_PROPAGATION_RULE_HPP
#define PACE2026_SINGLE_VERTEX_TREE_PROPAGATION_RULE_HPP

#include "AbstractRule.hpp"
#include "../Action/DeleteEdgeAction.hpp"
#include <unordered_set>
#include <stack>

namespace solver
{

/// \brief If there is terminal which is a single vertex tree in one forest,
/// the corresponding terminals in the other forests can be safely transformed into single vertex trees as well.
///
/// \see
/// <a href="https://gitlab.informatik.uni-bremen.de/pace-2026/orga/-/wikis/Branching/SingleVertexTreePropagationRule">
/// GitLab Documentation
/// </a>
class SingleVertexTreePropagationRule : public AbstractRule
{
  protected:

    /// \brief set of all labels where the corresponding terminals
    /// are somewhere but not everywhere single vertex trees
    std::unordered_set<unsigned int> labelsToBeReduced;


    /// \brief Stack of action that modify the instance,
    /// filled in the apply method and unfilled in the unapply method
    std::stack<DeleteEdgeAction> changes = std::stack<DeleteEdgeAction>();

public:
    /// \param instance the problem instance
    /// \param context information about the instance and the solver state
    /// \param labelsToBeReduced the position where the rule can be applied,\n
    /// which is set of all labels where the corresponding terminals
    /// are somewhere but not everywhere single vertex trees
    SingleVertexTreePropagationRule(const std::shared_ptr<graph::Instance>& instance,
                                    const std::shared_ptr<Context>& context,
                                    const std::unordered_set<unsigned int>& labelsToBeReduced);

    /// \brief applies rule
    /// \returns always \ref RuleReturnCode::Continue
    RuleReturnCode apply() override;

    void unapply() override;

    /// \brief It checks whether the SingleVertexTreePropagationRule is applicable
    /// and generates an instance of this rule if so.
    /// \param instance on which the rule should be applied
    /// \param context contains additional information to the instance and the solver state
    /// \returns shared_pointer to SingleVertexTreePropagationRule if rule is applicable, elso null pointer
    static std::shared_ptr<AbstractRule> isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                                      const std::shared_ptr<Context>& context);

    [[nodiscard]]
    std::string name() const override;

    [[nodiscard]]
    std::shared_ptr<AbstractRule> clone() const override;
};

}  //namespace solver

#endif  //PACE2026_SINGLE_VERTEX_TREE_PROPAGATION_RULE_HPP
