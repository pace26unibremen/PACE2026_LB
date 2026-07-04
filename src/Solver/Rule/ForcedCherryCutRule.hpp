#ifndef PACE2026_FORCED_CHERRY_CUT_RULE_HPP
#define PACE2026_FORCED_CHERRY_CUT_RULE_HPP

#include "AbstractRule.hpp"
#include "../Action/DeleteEdgeAction.hpp"

#include <stack>

namespace solver
{

/// \brief Approximation-track Case 3 of Whidden & Zeh's linear-time 3-approximation
/// (rSPR distance / rooted MAF, "A Unifying View on Approximation and FPT of Agreement
/// Forests", Section 4.1): if a sibling pair (a,c) of T1 = instance->at(0) is not a
/// sibling pair of T2 = instance->at(1), then a has a sibling b in T2 (guaranteed once
/// SingleVertexTreePropagationRule has run to fixpoint, since T2 then contains no
/// singletons). This rule cuts the edges to a, b, and c in T2 only, unconditionally —
/// there is no branch/choice, matching the paper exactly. T1 is left untouched;
/// SingleVertexTreePropagationRule mirrors the resulting T2 singletons into T1 for free
/// on a later pass.
///
/// \note Scoped to exactly 2 forests (instance->size() == 2) — the paper's proof and the
/// PACE 2026 heuristic/lower-bound tracks are both defined for pairs of trees.
class ForcedCherryCutRule : public AbstractRule
{
  protected:
    /// \brief label of the sibling pair member that is not the pivot of the T2 sibling
    unsigned int aLabel;

    /// \brief label of the other sibling pair member
    unsigned int cLabel;

    /// \brief Stack of actions that modify the instance,
    /// filled in the apply method and unfilled in the unapply method
    std::stack<DeleteEdgeAction> changes = std::stack<solver::DeleteEdgeAction>();

  public:
    /// \param instance the problem instance (must have exactly 2 forests)
    /// \param context information about the instance and the solver state
    /// \param aLabel label of one sibling-pair member in T1
    /// \param cLabel label of the other sibling-pair member in T1
    ForcedCherryCutRule(
        const std::shared_ptr<graph::Instance>& instance,
        const std::shared_ptr<Context>& context,
        unsigned int aLabel,
        unsigned int cLabel);

    /// \brief cuts e_a, e_b, e_c in T2 (instance->at(1)) only. Each cut is skipped if the
    /// node has already lost its parent as a side effect of an earlier cut in this call
    /// (e.g. cutting a sibling of a root promotes the other sibling to a root).
    /// \returns always \ref RuleReturnCode::Continue
    RuleReturnCode apply() override;

    void unapply() override;

    /// \brief Finds a sibling pair (a,c) in T1 that is not a sibling pair in T2.
    /// Only applicable for 2-forest instances.
    /// \param instance on which the rule should be applied
    /// \param context contains additional information to the instance and the solver state
    /// \returns shared_pointer to ForcedCherryCutRule if applicable, else null pointer
    static std::shared_ptr<AbstractRule> isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                                      const std::shared_ptr<Context>& context);

    [[nodiscard]]
    std::string name() const override;

    [[nodiscard]]
    std::shared_ptr<AbstractRule> clone() const override;
};

}  //namespace solver

#endif  //PACE2026_FORCED_CHERRY_CUT_RULE_HPP
