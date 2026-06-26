#include "ConvergencePlugin.hpp"

#include <utility>

solver::plugin::ConvergencePlugin::ConvergencePlugin(std::shared_ptr<MetricsCollector> collector,
                                                     std::ostream& out)
    : AbstractStridePlugin(out), collector(std::move(collector))
{
}

void solver::plugin::ConvergencePlugin::init(const std::shared_ptr<graph::Instance>& instance,
                                             const std::shared_ptr<solver::Context>& context)
{
    AbstractPlugin::init(instance, context);
    collector->startTime = std::chrono::steady_clock::now();
}

void solver::plugin::ConvergencePlugin::onNewBestSolution(float weight)
{
    snapshots.push_back(Snapshot{
        .wtime        = collector->elapsedSeconds(),
        .weight        = weight,
        .ruleCounts   = collector->ruleCounts,
        .branchOpens  = collector->branchOpens,
        .branchCloses = collector->branchCloses,
        .ruleTimes_ms = collector->ruleTimes_ms,
    });
}

void solver::plugin::ConvergencePlugin::onEnd()
{
    emitStrideLine("convergence", toJson(snapshots));
}
