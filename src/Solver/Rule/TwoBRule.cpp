#include "TwoBRule.hpp"

solver::TwoBRule::TwoBRule(const std::shared_ptr<graph::Instance>& instance, const std::shared_ptr<Context>& context,
                           const std::pair<unsigned int, unsigned int>& toCutLabelPair) :
        AbstractRule(instance, context, false),
        toCutLabel1(toCutLabelPair.first),
        toCutLabel2(toCutLabelPair.second)
{}

solver::RuleReturnCode solver::TwoBRule::apply()
{
    if (this->isApplied)
    {
        throw std::invalid_argument("TwoBRule : apply : rule is already applied");
    }
    isApplied = true;

    for (const auto& f : *instance)
    {
        const auto toCutNode1 = f->LabelToTerminal()[toCutLabel1];
        const auto toCutNode2 = f->LabelToTerminal()[toCutLabel2];
        changes.emplace(toCutNode1, f);
        changes.top().doAction();
        changes.emplace(toCutNode2, f);
        changes.top().doAction();
    }

    return RuleReturnCode::Continue;
}

void solver::TwoBRule::unapply()
{
    if (not this->isApplied)
    {
        throw std::invalid_argument("TwoBRule : unapply : rule is not applied");
    }
    isApplied = false;

    while (not changes.empty())
    {
        changes.top().undoAction();
        changes.pop();
        changes.top().undoAction();
        changes.pop();
    }
}

std::shared_ptr<solver::AbstractRule> solver::TwoBRule::isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                                                     const std::shared_ptr<Context>& context)
{
    std::pair<unsigned int, unsigned int> toCutLabelPair;
    const auto f1 = instance->at(0);
    unsigned int aLabel, cLabel, xLabel;

    for (const auto& [label, node] : f1->LabelToTerminal())
    {
        if (node->sibling != nullptr and f1->TerminalToLabel().contains(node->sibling))  //sibling (c) is leaf
        {
            if (node->parent->parent != nullptr and
                f1->TerminalToLabel().contains(node->parent->sibling))  //uncle (x) is leaf
            {
                aLabel = f1->TerminalToLabel().at(node);
                cLabel = f1->TerminalToLabel().at(node->sibling);
                xLabel = f1->TerminalToLabel().at(node->parent->sibling);

                bool doCut = true;

                auto f2 = instance->at(1);
                //get nodes a, c, x and check f2 for structure and memorize b1, b2 if possible
                graph::Node* a2 = f2->LabelToTerminal().at(aLabel);
                if (not(a2->parent != nullptr and a2->parent->parent != nullptr and
                        a2->parent->parent->parent != nullptr and a2->parent->parent->parent->parent != nullptr))
                {  // necessary structure does not exist in f2
                    doCut = false;
                }
                else if (not(a2->parent->parent->sibling == f2->LabelToTerminal().at(cLabel) and
                             a2->parent->parent->parent->sibling == f2->LabelToTerminal().at(xLabel)))
                {  // c and x are not leaves or aren't in the correct position
                    doCut = false;
                }
                if (not(f2->TerminalToLabel().contains(a2->sibling) and
                        f2->TerminalToLabel().contains(a2->parent->sibling)))
                {  // b1 and b2 aren't leaves
                    doCut = false;
                }
                else
                {
                    toCutLabelPair.first = f2->TerminalToLabel().at(a2->sibling);
                    toCutLabelPair.second = f2->TerminalToLabel().at(a2->parent->sibling);
                }

                for (unsigned int i = 2; i < instance->size(); ++i)
                {
                    if (not doCut)
                    {
                        break;
                    }
                    const auto fi = instance->at(i);

                    graph::Node* a = fi->LabelToTerminal().at(aLabel);
                    graph::Node* c = fi->LabelToTerminal().at(cLabel);
                    graph::Node* x = fi->LabelToTerminal().at(xLabel);

                    bool doShortCut = true;
                    bool doLongCut = true;

                    // block dealing with the shorter case
                    if (not(a->parent != nullptr and a->parent->parent != nullptr))
                    {  // slightly redundant checking kept for readability
                        doShortCut = false;
                    }
                    else if (not(a->sibling == c and a->parent->sibling == x))
                    {
                        doShortCut = false;
                    }
                    // block over

                    // block dealing with the longer case
                    if (not(a->parent != nullptr and a->parent->parent != nullptr and
                            a->parent->parent->parent != nullptr and a->parent->parent->parent->parent != nullptr))
                    {  // structure is not found
                        doLongCut = false;
                    }
                    else if (not(a->parent->parent->sibling == c and a->parent->parent->parent->sibling == x))
                    {  // node(s) c or x wrong
                        doLongCut = false;
                    }
                    if (not(fi->TerminalToLabel().contains(a->sibling) and
                            fi->TerminalToLabel().contains(a->parent->sibling)))
                    {  // b1 or b2 aren't leaves
                        doLongCut = false;
                    }
                    else if (not(fi->TerminalToLabel().at(a->sibling) == toCutLabelPair.first and
                                 fi->TerminalToLabel().at(a->parent->sibling) == toCutLabelPair.second))
                    {  // leaves to be cut match those of previous forests
                        doLongCut = false;
                    }
                    // block over

                    // cut still possible if either short or long cut applicable
                    //TODO: restore this doCut = doShortCut or doLongCut;
                    doCut = doLongCut;
                }

                if (doCut)
                {
                    return std::make_shared<TwoBRule>(instance, context, toCutLabelPair);
                }
            }
        }
    }
    return nullptr;
}

std::string solver::TwoBRule::name() const
{
    return "TwoBRule";
}

std::shared_ptr<solver::AbstractRule> solver::TwoBRule::clone() const
{
    return std::make_shared<TwoBRule>(instance, context, std::pair(toCutLabel1, toCutLabel2));
}
