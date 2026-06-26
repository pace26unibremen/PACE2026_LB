#include "SingleVertexTreePropagationRule.hpp"

solver::SingleVertexTreePropagationRule::SingleVertexTreePropagationRule(
    const std::shared_ptr<graph::Instance>& instance,
    const std::shared_ptr<Context>& context,
    const std::unordered_set<unsigned int>& labelsToBeReduced) :
        AbstractRule(instance, context, false),
        labelsToBeReduced(labelsToBeReduced)
{}

solver::RuleReturnCode solver::SingleVertexTreePropagationRule::apply()
{
    if (this->isApplied)
    {
        throw std::invalid_argument("SingleVertexTreePropagationRule : apply : rule is already applied");
    }
    isApplied = true;

    for (const auto& fi : *instance)
    {
        for (unsigned int label : labelsToBeReduced)
        {
            graph::Node* terminal = fi->LabelToTerminal()[label];
            if (terminal->parent != nullptr)
            {
                if (context->protectedEdges.contains(terminal))
                {
                    return RuleReturnCode::CutBranch;
                }
                changes.emplace(terminal, fi);
                changes.top().doAction();
            }
        }
    }

    return RuleReturnCode::Continue;
}

void solver::SingleVertexTreePropagationRule::unapply()
{
    if (not this->isApplied)
    {
        throw std::invalid_argument("SingleVertexTreePropagationRule : unapply : rule is not applied");
    }
    isApplied = false;

    while (not changes.empty())
    {
        changes.top().undoAction();
        changes.pop();
    }
}

std::shared_ptr<solver::AbstractRule>
solver::SingleVertexTreePropagationRule::isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                                      const std::shared_ptr<Context>& context)
{
    auto labelsToBeReduced = std::unordered_set<unsigned int>();

    for (const auto& [_, label] : instance->at(0)->TerminalToLabel())
    {
        bool anySingleVertexTree = false;
        bool anyNotSingleVertexTree = false;

        for (const auto& f_ptr : *instance)
        {
            const graph::Node* terminal = f_ptr->LabelToTerminal()[label];
            if (terminal->parent != nullptr)
            {
                anyNotSingleVertexTree = true;
            }
            else
            {
                anySingleVertexTree = true;
            }
        }
        if (anySingleVertexTree and anyNotSingleVertexTree)
        {
            labelsToBeReduced.insert(label);
        }
    }

    if(labelsToBeReduced.empty())
    {
        return nullptr;
    }
    return std::make_shared<SingleVertexTreePropagationRule>(instance, context, labelsToBeReduced);
}

std::string solver::SingleVertexTreePropagationRule::name() const
{
    return "SingleVertexTreePropagationRule";
}

std::shared_ptr<solver::AbstractRule> solver::SingleVertexTreePropagationRule::clone() const
{
    return std::make_shared<SingleVertexTreePropagationRule>(instance, context, labelsToBeReduced);
}
