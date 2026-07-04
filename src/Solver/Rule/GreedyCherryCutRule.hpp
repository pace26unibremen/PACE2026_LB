#ifndef PACE2026_GREEDY_CHERRY_CUT_RULE_HPP
#define PACE2026_GREEDY_CHERRY_CUT_RULE_HPP

#include "AbstractRule.hpp"
#include "../Action/DeleteEdgeAction.hpp"

#include <stack>

namespace solver
{

/// \brief Greedy multi-tree (t >= 2) approximation step for the Maximum Agreement Forest.
///
/// This is the general-\c t counterpart of \ref ForcedCherryCutRule. The pairwise linear-time
/// 3-approximation of Whidden & Zeh (Case 3, implemented by \ref ForcedCherryCutRule) is only
/// defined for two trees; for \c t > 2 there is no equally cheap constant-factor rule, so we fall
/// back to a simple, provably valid greedy step:
///
/// > Pick a cherry \c (a,c) of the reference forest \c f0 = \c instance->at(0). If \c (a,c) is
/// > \b not a common cherry across all \c t forests (i.e. \c a and \c c are not siblings in at
/// > least one forest), isolate one of the two leaves — here \c a — by cutting its pendant edge
/// > in \b every forest.
///
/// After the step \c a is a singleton in all forests, which is trivially a valid block of any
/// agreement forest, so the conflict at \c (a,c) is resolved and the instance is strictly smaller.
/// Iterating this (interleaved with the free rules \ref CheckSingleVertexTreesRule /
/// \ref SingleVertexTreePropagationRule / \ref EqualPairReductionRule) therefore always terminates
/// in a \b valid agreement forest for any number of trees.
///
/// \par What is (and is not) guaranteed
/// - \b Validity holds for any \c t >= 2: the construction only ever deletes edges, and every leaf
///   ends up in a block that induces the same (single-leaf or common) subtree in all trees, so the
///   result is a genuine agreement forest — i.e. a valid \b upper \b bound on the optimum.
/// - \b No \b approximation \b ratio is claimed for \c t > 2. Cutting a single leaf per conflict can
///   be far from optimal, so this is a heuristic upper bound, intended primarily to seed the
///   branch-and-bound incumbent (\ref Context::bestSolutionWeight) — not to be a competitive
///   standalone answer. For \c t == 2 the sharper \ref ForcedCherryCutRule runs instead (it has
///   priority in the approximation rule list), so this rule is only ever reached for \c t > 2.
///
/// \note Constructed non-reduction (\c isReduction == \c false): the cut is part of the constructed
///       solution and must \b not be undone by \ref AbstractSolver::unapplyReductions.
class GreedyCherryCutRule : public AbstractRule
{
  protected:
    /// \brief Label of the cherry member that is isolated (cut out of every forest).
    unsigned int aLabel;

    /// \brief Label of the other cherry member (kept in place; retained for \ref clone / debugging).
    unsigned int cLabel;

    /// \brief Actions applied by \ref apply, unwound in reverse by \ref unapply.
    std::stack<DeleteEdgeAction> changes = std::stack<solver::DeleteEdgeAction>();

  public:
    /// \param instance the problem instance (any number of forests)
    /// \param context solver state (used for \ref Context::protectedEdges)
    /// \param aLabel label of the cherry member to isolate
    /// \param cLabel label of the other cherry member
    GreedyCherryCutRule(const std::shared_ptr<graph::Instance>& instance,
                        const std::shared_ptr<Context>& context,
                        unsigned int aLabel,
                        unsigned int cLabel);

    /// \brief Cuts the pendant edge of \c aLabel in every forest in which it still has a parent.
    /// \returns \ref RuleReturnCode::Continue (or \ref RuleReturnCode::CutBranch if an edge that
    ///          would have to be cut is protected — not expected on the approximation path).
    RuleReturnCode apply() override;

    void unapply() override;

    /// \brief Applicable iff \c f0 has a cherry \c (a,c) that is not a common cherry of all forests.
    /// (For \c t == 2, \ref ForcedCherryCutRule takes precedence in the rule list, so in practice
    ///  this fires only for \c t > 2.)
    static std::shared_ptr<AbstractRule> isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                                      const std::shared_ptr<Context>& context);

    [[nodiscard]]
    std::string name() const override;

    [[nodiscard]]
    std::shared_ptr<AbstractRule> clone() const override;
};

}  //namespace solver

#endif  //PACE2026_GREEDY_CHERRY_CUT_RULE_HPP
