#include "RuleStatsPlugin.hpp"

#include <utility>

solver::plugin::RuleStatsPlugin::RuleStatsPlugin(std::shared_ptr<MetricsCollector> collector,
                                                  bool collectTiming,
                                                  std::ostream& out)
    : AbstractStridePlugin(out), collector(std::move(collector)), collectTiming(collectTiming)
{
}

void solver::plugin::RuleStatsPlugin::beforeApply(const std::shared_ptr<solver::AbstractRule>&)
{
    if (collectTiming)
        ruleStart = std::chrono::steady_clock::now();
}

void solver::plugin::RuleStatsPlugin::onApply(const std::shared_ptr<solver::AbstractRule>& rule)
{
    const std::string& key = rule->name();
    collector->ruleCounts[key]++;
    if (collectTiming)
    {
        using namespace std::chrono;
        const double elapsedMs = duration<double, std::milli>(steady_clock::now() - ruleStart).count();
        collector->ruleTimes_ms[key] += elapsedMs;
    }
}

void solver::plugin::RuleStatsPlugin::onEnd()
{
    // Keys in the collector are CamelCase (rule->name()); convert to snake_case
    // only here at emit time, not on every onApply call in the hot path.
    emitStrideLine("rule_times_ms", toJsonSnakeKeys(collector->ruleTimes_ms));
    emitStrideLine("rule_counts",   toJsonSnakeKeys(collector->ruleCounts));
}
