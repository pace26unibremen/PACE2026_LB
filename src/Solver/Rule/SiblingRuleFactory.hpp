#ifndef PACE2026_SIBLING_RULE_FACTORY_HPP
#define PACE2026_SIBLING_RULE_FACTORY_HPP

#include "AbstractRule.hpp"

namespace solver
{

/// \brief This class provides combined `isApplicable` functions for the sibling rules.
class SiblingRuleFactory
{
  private:
    /// \brief Selects a sibling pair from the first forest of the instance.
    /// \param instance
    /// \param context
    /// \return The labels of a sibling pair
    static std::pair<unsigned int, unsigned int>
    getSiblings(const std::shared_ptr<graph::Instance>& instance,
                const std::shared_ptr<solver::Context>& context);

    /// \brief For two nodes \ref aNode and \ref cNode if they are part of a local pendant chain.
    /// - case 1: aNode and cNode are separated by one subtree with subtree root b
    /// - case 2: aNode and cNode are separated by two subtrees with subtree roots b1 and b2
    /// - case 3: else
    /// \param aNode
    /// \param cNode
    /// \returns A pair of the two possible subtree roots between \ref aNode and \ref cNode
    /// - case 1: {b, nullptr}
    /// - case 2: {b1, b2}
    /// - case 3: {nullptr, nullptr}
    static std::pair<graph::Node*, graph::Node*>
    checkBNodes(graph::Node* const & aNode, graph::Node* const & cNode);

  public:
    /// \brief Checks whether \ref EqualPairReductionRule, \ref ACBranchingRule or \ref ABCBranchingRule
    /// can be applied and returns an instance of the best applicable rule,
    /// with the rules prioritized in the following order:
    /// 1. \ref ACBranchingRule
    /// 2. \ref EqualPairReductionRule
    /// 3. \ref ABCBranchingRule.
    /// If none of the rules is applicable the method returns a null pointer.
    /// \param instance on which the rule should be applied
    /// \param context contains additional information to the instance and the solver state
    /// \returns shared pointer to an instance of the best applicable rule
    static std::shared_ptr<AbstractRule> basicRules(const std::shared_ptr<graph::Instance>& instance,
                                                    const std::shared_ptr<solver::Context>& context);

    /// \brief Checks whether a 'sibling rule' can be applied and returns an instance of the best applicable rule,
    /// with the rules prioritized in the following order:
    /// 1. \ref ACBranchingRule
    /// 2. \ref EqualPairReductionRule
    /// 3. \ref BRule
    /// 4. \ref ReverseBRule
    /// 5. \ref TwoBRule
    /// 6. \ref ABCBranchingRule.
    /// If none of the rules is applicable the method returns a null pointer.
    /// \param instance on which the rule should be applied
    /// \param context contains additional information to the instance and the solver state
    /// \returns shared pointer to an instance of the best applicable rule
    static std::shared_ptr<AbstractRule> allRules(const std::shared_ptr<graph::Instance>& instance,
                                                  const std::shared_ptr<solver::Context>& context);
};

}  //namespace solver

#endif  //PACE2026_SIBLING_RULE_FACTORY_HPP
