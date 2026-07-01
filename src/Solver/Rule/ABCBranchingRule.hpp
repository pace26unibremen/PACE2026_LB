#ifndef PACE2026_ABC_BRANCHING_RULE_HPP
#define PACE2026_ABC_BRANCHING_RULE_HPP

#include "../Action/DeleteEdgeAction.hpp"
#include "AbstractBranchingRule.hpp"

#include <list>
#include <stack>

namespace solver
{

/// \brief If there is a pair of two terminals in one forest and in some other forests of the instance
/// the corresponding terminals aren't siblings (but are connected), we can branch into three cases:\n
/// 1. The first terminal is a single vertex tree in the solution. -> cut this terminal
/// 2. The other terminal is a single vertex tree in the solution. -> cut the other terminal
/// 3. The two terminals are a pair in the solution. -> cut all subtrees on the path between both terminals
///
/// \see
/// <a href="https://gitlab.informatik.uni-bremen.de/pace-2026/orga/-/wikis/Branching/PairPathBranchingRule">
/// GitLab Documentation
/// </a>
class ABCBranchingRule : public AbstractBranchingRule
{
  protected:
    /// \brief label of the a-nodes
    unsigned int aLabel;

    /// \brief label of the c-nodes
    unsigned int cLabel;

    /// \brief a list of forests with theirs b-nodes
    std::list<std::pair<std::shared_ptr<graph::Forest>, std::list<graph::Node*>>> forestWithBNodes;

    /// \brief Stack of action that modify the instance,
    /// filled in the apply method and unfilled in the unapply method
    std::stack<DeleteEdgeAction> changes = std::stack<DeleteEdgeAction>();

    /// \brief A list of rules that should be applied next.
    std::shared_ptr<std::list<std::shared_ptr<AbstractRule>>> nextRuleSuggestion =
        std::make_shared<std::list<std::shared_ptr<AbstractRule>>>();

    /// \brief A set of all edges that get protection in branch 3.
    /// An edge is identified by the node it points to (the child-node).
    std::unordered_set<graph::Node*> edgeProtections = std::unordered_set<graph::Node*>();

  public:
    /// \param instance the problem instance
    /// \param context information about the instance and the solver state
    /// \param aLabel label of the a-nodes
    /// \param cLabel label of the c-nodes
    /// \param forestWithBNodes a list of forests with theirs b-nodes
    ABCBranchingRule(
        const std::shared_ptr<graph::Instance>& instance,
        const std::shared_ptr<Context>& context,
        unsigned int aLabel,
        unsigned int cLabel,
        const std::list<std::pair<std::shared_ptr<graph::Forest>, std::list<graph::Node*>>>& forestWithBNodes);


    /// \brief applies rule
    /// \returns two return codes are possible:
    /// - \ref RuleReturnCode::Continue, on branch 2 and 3
    /// - \ref RuleReturnCode::ContinueWithRuleSuggestion, on branch 1 (suggests a \ref PairEqualRule)
    RuleReturnCode apply() override;

    void unapply() override;

    /// \brief It checks whether the ABCBranchingRule is applicable and generates an instance of this rule if so.
    /// This method only considers the ABCBranchingRule applicable if the first pair it finds in forest 1
    /// has corresponding terminals in another forest with a non-trivial path and are nowhere disconnected.
    /// \param instance on which the rule should be applied
    /// \param context contains additional information to the instance and the solver state
    /// \returns shared_pointer to ABCBranchingRule if rule is applicable, elso null pointer
    static std::shared_ptr<AbstractRule> isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                                      const std::shared_ptr<Context>& context);


    std::shared_ptr<std::list<std::shared_ptr<AbstractRule>>> NextRuleSuggestion() override;

    [[nodiscard]]
    std::string name() const override;

    [[nodiscard]]
    std::shared_ptr<AbstractRule> clone() const override;
};

}  //namespace solver

#endif  //PACE2026_ABC_BRANCHING_RULE_HPP
