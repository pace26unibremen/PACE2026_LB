#ifndef PACE2026_CUT_BRANCH_RULE_HPP
#define PACE2026_CUT_BRANCH_RULE_HPP

#include "AbstractRule.hpp"

namespace solver
{

/// \brief This rule instructs the solver to cut the current branch.
/// It works differently depending on whether the solver performs a bounded depth search
/// (\ref BranchingSolverConfiguration::boundedDephtSearch).
/// 1. With a bounded depth search, the branch will be cut if
/// the current instance is worse than the \ref Context::maxSolutionSize parameter.
/// 2. Without a bounded depth search, the branch will be cut if
/// the current instance is already equal to or worse than to the current solution.
class CutBranchRule : public  AbstractRule
{
  public:
    CutBranchRule(const std::shared_ptr<graph::Instance>& instance,
                  const std::shared_ptr<Context>& context);

    /// \brief applies rule
    /// \returns always \ref RuleReturnCode::CutBranch
    RuleReturnCode apply() override;

    void unapply() override;

    /// \brief It checks whether the CutBranchRule is applicable and generates an instance of this rule if so.
    /// This method considers the CutBranchRule applicable if:
    /// 1. _With a bounded depth search_,
    /// the number of trees in a forest are greater than the \ref Context::maxSolutionSize parameter.
    /// 2. _Without a bounded depth search_,
    /// the number of trees in a forest are greater than or equal to \ref Context::bestSolutionSize.
    /// \param instance on which the rule should be applied
    /// \param context contains additional information to the instance and the solver state
    /// \returns shared_pointer to CutBranchRule if rule is applicable, else null pointer
    static std::shared_ptr<AbstractRule> isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                                      const std::shared_ptr<Context>& context);

    [[nodiscard]]
    std::string name() const override;

    [[nodiscard]]
    std::shared_ptr<AbstractRule> clone() const override;
};

}  //namespace solver

#endif  //PACE2026_CUT_BRANCH_RULE_HPP
