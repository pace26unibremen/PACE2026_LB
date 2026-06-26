#ifndef PACE2026_METRICS_PLUGINS_HPP
#define PACE2026_METRICS_PLUGINS_HPP

#include "AbstractPlugin.hpp"
#include "BranchCountPlugin.hpp"
#include "ConvergencePlugin.hpp"
#include "MetricsCollector.hpp"
#include "RuleStatsPlugin.hpp"

#include <memory>
#include <vector>

namespace solver::plugin
{

/// \brief Factory for the full metrics plugin suite.
///
/// Creates one \ref RuleStatsPlugin, one \ref BranchCountPlugin, and one
/// \ref ConvergencePlugin, all wired to the same \ref MetricsCollector
/// instance.  This is required for consistent snapshots in the convergence
/// trace: the collector's rule-count, timing, and branch data must reflect
/// exactly what the other two plugins have accumulated up to each improvement
/// point.
///
/// Usage:
/// \code
///   auto config = std::make_shared<BranchingSolverConfiguration>();
///   for (auto& p : MetricsPlugins::makeAll())
///       config->plugins.push_back(p);
/// \endcode
///
/// Individual plugins can still be constructed standalone when only a single
/// metric dimension is needed (e.g. \ref BranchCountPlugin alone).
struct MetricsPlugins
{
    /// \brief Create all three metric plugins sharing one \ref MetricsCollector.
    /// \param out Stream to write \c "#s …" lines to.  Defaults to \c std::cout.
    /// \return Vector of three plugins ready to be appended to
    ///         \ref BranchingSolverConfiguration::plugins.
    static std::vector<std::shared_ptr<AbstractPlugin>> makeAll(std::ostream& out = std::cout)
    {
        auto collector = std::make_shared<MetricsCollector>();
        return {
            std::make_shared<RuleStatsPlugin>(collector, true, out),
            std::make_shared<BranchCountPlugin>(collector, out),
            std::make_shared<ConvergencePlugin>(collector, out),
        };
    }
};

} // namespace solver::plugin

#endif  //PACE2026_METRICS_PLUGINS_HPP
