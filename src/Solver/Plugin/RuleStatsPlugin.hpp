#ifndef PACE2026_RULE_STATS_PLUGIN_HPP
#define PACE2026_RULE_STATS_PLUGIN_HPP

#include "AbstractStridePlugin.hpp"
#include "MetricsCollector.hpp"

#include <chrono>
#include <memory>

namespace solver::plugin
{

/// \brief Plugin that records per-rule application counts and optionally wall-clock times.
///
/// In \c beforeApply / \c onApply it counts each rule call and, when
/// \p collectTiming is \c true, also measures elapsed time.  Both are
/// accumulated into the shared \ref MetricsCollector.  On \c onEnd() it emits
/// two stride lines:
/// \code
///   #s rule_times_ms {"RuleName":N.NNN,...}   (empty map {} when timing is off)
///   #s rule_counts   {"RuleName":N,...}
/// \endcode
class RuleStatsPlugin : public AbstractStridePlugin
{
    std::shared_ptr<MetricsCollector> collector;

    /// \brief Whether wall-clock timing is active. Set at construction time.
    bool collectTiming;

    /// \brief Timestamp taken in \c beforeApply; used by the following \c onApply.
    /// Only meaningful when \c collectTiming is \c true.
    std::chrono::steady_clock::time_point ruleStart;

  public:
    /// \param collector  shared metrics accumulator
    /// \param collectTiming  if \c true (default) wall-clock time is measured per
    ///                       rule application; if \c false only counts are collected,
    ///                       eliminating the timer overhead.
    explicit RuleStatsPlugin(std::shared_ptr<MetricsCollector> collector,
                             bool collectTiming = true,
                             std::ostream& out = std::cout);

    void beforeApply(const std::shared_ptr<solver::AbstractRule>& rule) override;
    void onApply(const std::shared_ptr<solver::AbstractRule>& rule) override;
    void onEnd() override;
};

} // namespace solver::plugin

#endif  //PACE2026_RULE_STATS_PLUGIN_HPP
