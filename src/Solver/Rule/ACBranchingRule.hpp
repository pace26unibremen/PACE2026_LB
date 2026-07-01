#ifndef PACE2026_AC_BRANCHING_RULE_HPP
#define PACE2026_AC_BRANCHING_RULE_HPP

#include "AbstractBranchingRule.hpp"
#include "../Action/DeleteEdgeAction.hpp"

#include <list>
#include <stack>

namespace solver
{

/// \brief If there is a pair of two terminals in one forest and in some other forests of the instance
/// the corresponding terminals aren't connected (are in different trees), we can branch into two cases:\n
/// 1. The first terminal is a single vertex tree in the solution. -> cut this terminal
/// 2. The other terminal is a single vertex tree in the solution. -> cut the other terminal
///
/// \see
/// <a href="https://gitlab.informatik.uni-bremen.de/pace-2026/orga/-/wikis/Branching/PairUnconnectedBranchingRule">
/// GitLab Documentation
/// </a>
class ACBranchingRule : public AbstractBranchingRule
{
  protected:

    /// \brief label of the a-node
    unsigned int aLabel;

    /// \brief label of the c-node
    unsigned int cLabel;

    /// \brief Stack of action that modify the instance,
    /// filled in the apply method and unfilled in the unapply method
    std::stack<DeleteEdgeAction> changes = std::stack<solver::DeleteEdgeAction>();

    /// \brief A set of all edges that get protection in branch 2.
    /// An edge is identified by the node it points to (the child-node).
    std::unordered_set<graph::Node*> edgeProtections = std::unordered_set<graph::Node*>();

  public:
    /// \param instance the problem instance
    /// \param context information about the instance and the solver state
    /// \param aLabel label of the a-node
    /// \param cLabel label of the c-node
    ACBranchingRule(
        const std::shared_ptr<graph::Instance>& instance,
        const std::shared_ptr<Context>& context,
        unsigned int aLabel,
        unsigned int cLabel);

    /// \brief applies rule
    /// \returns always \ref RuleReturnCode::Continue
    RuleReturnCode apply() override;

    void unapply() override;

    /// \brief It checks whether the ACBranchingRule is applicable and generates an instance of this rule if so.
    /// This method only considers the ACBranchingRule applicable if the first pair it finds in forest 1
    /// has corresponding terminals in another forest that are disconnected.
    /// \param instance on which the rule should be applied
    /// \param context contains additional information to the instance and the solver state
    /// \returns shared_pointer to ACBranchingRule if rule is applicable, elso null pointer
    static std::shared_ptr<AbstractRule> isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                                      const std::shared_ptr<Context>& context);

    [[nodiscard]]
    std::string name() const override;

    [[nodiscard]]
    std::shared_ptr<AbstractRule> clone() const override;
};

}  //namespace solver

#endif  //PACE2026_AC_BRANCHING_RULE_HPP
