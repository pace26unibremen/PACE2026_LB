#include "BranchCountPlugin.hpp"

#include "../Rule/AbstractBranchingRule.hpp"

#include <utility>

solver::plugin::BranchCountPlugin::BranchCountPlugin(std::shared_ptr<MetricsCollector> collector,
                                                     std::ostream& out)
    : AbstractStridePlugin(out), collector(std::move(collector))
{
}

void solver::plugin::BranchCountPlugin::onApply(const std::shared_ptr<solver::AbstractRule>& rule)
{
    if (dynamic_cast<solver::AbstractBranchingRule*>(rule.get()) != nullptr)
    {
        collector->branchOpens++;
    }
}

void solver::plugin::BranchCountPlugin::onBranchEnd()
{
    collector->branchCloses++;
}

void solver::plugin::BranchCountPlugin::onEnd()
{
    emitStrideLine("branch_opens", std::to_string(collector->branchOpens));
    emitStrideLine("branch_closes", std::to_string(collector->branchCloses));
}
