#include "CheckSingleVertexTreesRule.hpp"


solver::CheckSingleVertexTreesRule::CheckSingleVertexTreesRule(const std::shared_ptr<graph::Instance>& instance,
                                     const std::shared_ptr<Context>& context) :
    AbstractRule(instance,context, false)
{
}


solver::RuleReturnCode solver::CheckSingleVertexTreesRule::apply()
{
    if (this->isApplied)
    {
        throw std::invalid_argument("CheckSingleVertexTreesRule : apply : rule is already applied");
    }
    isApplied = true;

    return RuleReturnCode::EndBranchWithSolutionCandidate;
}

void solver::CheckSingleVertexTreesRule::unapply()
{
    if (not this->isApplied)
    {
        throw std::invalid_argument("CheckSingleVertexTreesRule : unapply : rule is not applied");
    }
    isApplied = false;
}


std::shared_ptr<solver::AbstractRule>
solver::CheckSingleVertexTreesRule::isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                    const std::shared_ptr<Context>& context)
{
    auto f = instance->at(0);

    if (f->Roots().size() == f->TerminalToLabel().size())
    {
        return std::make_shared<CheckSingleVertexTreesRule>(instance, context);
    }
    return nullptr;
}

std::string solver::CheckSingleVertexTreesRule::name() const
{
    return "CheckSingleVertexTreesRule";
}

std::shared_ptr<solver::AbstractRule> solver::CheckSingleVertexTreesRule::clone() const
{
    return std::make_shared<CheckSingleVertexTreesRule>(instance, context);
}
