#include "AbstractPlugin.hpp"

// Default no-op implementations. Subclasses override only the hooks they need.
// init() additionally stores instance and context so subclasses can access them
// via instance_ and context_ without re-declaring those fields.

void solver::plugin::AbstractPlugin::init(const std::shared_ptr<graph::Instance>& instance,
                                          const std::shared_ptr<solver::Context>& context)
{
    instance_ = instance;
    context_  = context;
}

void solver::plugin::AbstractPlugin::beforeApply(const std::shared_ptr<solver::AbstractRule>&) {}

void solver::plugin::AbstractPlugin::onApply(const std::shared_ptr<solver::AbstractRule>&) {}

void solver::plugin::AbstractPlugin::onUnapply(const std::shared_ptr<solver::AbstractRule>&) {}

void solver::plugin::AbstractPlugin::onReductionUnapply(const std::shared_ptr<solver::AbstractRule>&, bool) {}

void solver::plugin::AbstractPlugin::onReductionReapply(const std::shared_ptr<solver::AbstractRule>&) {}

void solver::plugin::AbstractPlugin::onNewBestSolution(float) {}

void solver::plugin::AbstractPlugin::onBranchEnd() {}

void solver::plugin::AbstractPlugin::onEnd() {}
