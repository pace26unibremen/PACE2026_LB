#ifndef PACE2026_CHAIN_REDUCTION_RULE_HPP
#define PACE2026_CHAIN_REDUCTION_RULE_HPP

#include "../Action/ShortenChainAction.hpp"
#include "AbstractRule.hpp"

#include <stack>
#include <vector>

namespace solver
{
/// \brief The chain reduction rule shortens common chains of terminals.
///
/// \see
/// <a href="https://gitlab.informatik.uni-bremen.de/pace-2026/orga/-/wikis/Branching/ChainReductionRule">
/// GitLab Documentation
/// </a>
class ChainReductionRule : public AbstractRule
{
  protected:
    /// \brief a list of the lower and upper label for each chain.
    /// first element of the pair is 'lower'
    /// second element of the pair is 'upper'
    /// \note this represents not the whole chain, but the part that will be reduced.
    std::list<std::pair<unsigned int, unsigned int>> reducedChains;

    /// \brief the ShortenChainAction
    std::stack<ShortenChainAction> changes;

  public:
    /// \brief Chain Reduction Rule implementation that identifies the first chain out of the two forests given
    /// \param instance The problem instance
    /// \param context information about the instance and the solver state
    /// \param reducedChains A list of the reduce-able chain parts, as pairs of [lower label, upper label].
    ChainReductionRule(const std::shared_ptr<graph::Instance>& instance,
                       const std::shared_ptr<Context>& context,
                       const std::list<std::pair<unsigned int, unsigned int>>& reducedChains);

    /// \brief Apply the Chain Reduction rule onto the two Trees
    RuleReturnCode apply() override;

    /// \brief Restore the original state before the application of the chain reduction rule
    void unapply() override;

    /// \brief Determine if there are chains between the two trees.
    static std::shared_ptr<AbstractRule> isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                                      const std::shared_ptr<Context>& context);

    [[nodiscard]] std::string name() const override;

    [[nodiscard]] std::shared_ptr<AbstractRule> clone() const override;
};

}  //namespace solver

#endif  //PACE2026_CHAIN_REDUCTION_RULE_HPP
