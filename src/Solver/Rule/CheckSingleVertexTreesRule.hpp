#ifndef PACE2026_CHECK_SINGLE_VERTEX_TREES_RULE_HPP
#define PACE2026_CHECK_SINGLE_VERTEX_TREES_RULE_HPP

#include "AbstractRule.hpp"


namespace solver
{

/// \brief Checks if the first forest of the instance consists of only leaves, i.e. single vertex trees.
///
/// \see
/// <a href="https://gitlab.informatik.uni-bremen.de/pace-2026/orga/-/wikis/Branching/CheckSingleVertexTreesRule">
/// GitLab Documentation

/// </a>
class CheckSingleVertexTreesRule : public AbstractRule
{
  public:
    /// \param instance the problem instance
    /// \param context information about the instance and the solver state
    /// which is a mapping of each forest to the subtree in this forest that can be collapsed
    CheckSingleVertexTreesRule(const std::shared_ptr<graph::Instance>& instance,
                  const std::shared_ptr<Context>& context);

    /// \brief applies rule
    /// \returns \ref RuleReturnCode::EndBranchWithSolutionCandidate if the first forest consists only of single vertex trees
    RuleReturnCode apply() override;

    void unapply() override;

    /// \brief It checks whether the CheckSingleVertexTreesRule is applicable and generates an instance of this rule if so.
    /// Importantly, may terminate search even if some forests still have connected components as long as the first forest
    /// of the instance only contains single vertex trees.
    /// \param instance on which the rule should be applied
    /// \param context contains additional information to the instance and the solver state
    /// \returns shared_pointer to CheckSingleVertexTreesRule if rule is applicable, else null pointer
    static std::shared_ptr<AbstractRule> isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                                      const std::shared_ptr<Context>& context);

    [[nodiscard]]
    std::string name() const override;

    [[nodiscard]]
    std::shared_ptr<AbstractRule> clone() const override;
};

}  //namespace solver

#endif  //PACE2026_CHECK_SINGLE_VERTEX_TREES_RULE_HPP
