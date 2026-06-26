#include "DebugAssertFalseRule.hpp"

solver::DebugAssertFalseRule::DebugAssertFalseRule(const std::shared_ptr<graph::Instance>& instance,
                                                   const std::shared_ptr<Context>& context) :
    AbstractRule(instance, context, false)
{}

solver::RuleReturnCode solver::DebugAssertFalseRule::apply()
{
    return RuleReturnCode::ImidateReturn;
}

void solver::DebugAssertFalseRule::unapply() {}

std::shared_ptr<solver::AbstractRule>
solver::DebugAssertFalseRule::isApplicable(const std::shared_ptr<graph::Instance>& instance,
                                           const std::shared_ptr<Context>& context)
{
    return std::make_shared<DebugAssertFalseRule>(instance, context);
}

std::string solver::DebugAssertFalseRule::name() const
{
    return "DebugAssertFalseRule";
}

std::shared_ptr<solver::AbstractRule> solver::DebugAssertFalseRule::clone() const
{
    return std::make_shared<DebugAssertFalseRule>(instance, context);
}
