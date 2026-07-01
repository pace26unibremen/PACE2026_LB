//
// Created by kaufm on 12.02.2026.
//

#ifndef PACE2026_B_RULE_HPP
#define PACE2026_B_RULE_HPP

#include "../Action/DeleteEdgeAction.hpp"
#include "AbstractRule.hpp"

#include <stack>
#include <vector>

namespace solver
{

class BRule : public AbstractRule
{
  protected:
    /// \brief The Label of the B node that should be deleted
    unsigned b1Label;
    /// \brief The label of a potential alternative B node, can be 0
    unsigned b2Label;
    /// \brief A stack entailing all edge deletion actions between the B-Nodes and their respective forests
    std::stack<DeleteEdgeAction> changes;

  public:
    /// \brief Constructor for the B-Rule
    /// \see Whidden et al. 2013
    /// \param instance The instance which is to be analyzed
    /// \param context Information storage for the branches
    /// \param b1Label The label of the B node
    /// \param b2Label The label of a potential alternative B node, can be 0.
    BRule(const std::shared_ptr<graph::Instance>& instance,
          const std::shared_ptr<Context>& context,
          unsigned int b1Label,
          unsigned int b2Label);

    /// \brief Applies the B-Rule onto all forests within the instance
    RuleReturnCode apply() override;

    /// \brief Reverses the B-Rule appliance on the instance
    void unapply() override;

    /// \brief Checks if for two param nodes there's a path of length 3 between them
    /// \param aNode The first node
    /// \param cNode The second node
    /// \returns If there is a length 3 path, returns the sibling of the whichever node is lower within the tree, else
    /// it returns a nullptr
    ///
    static graph::Node* isA3Path(const graph::Node* aNode, const graph::Node* cNode);

    /// \brief Main Function to identify if the B Rule is applicable to the current instance meant to be solved
    /// \param instance the instance containing all the trees
    /// \param context the respective information for the instance#
    /// \returns A shared pointer containing the parameters as well as the b node in form of a label if the B Rule is
    /// true for the current instance, otherwise nullptr
    static std::shared_ptr<AbstractRule> isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                                      const std::shared_ptr<Context>& context);
    [[nodiscard]]
    std::string name() const override;

    [[nodiscard]]
    std::shared_ptr<AbstractRule> clone() const override;
};

}  //namespace solver

#endif  //PACE2026_B_RULE_HPP
