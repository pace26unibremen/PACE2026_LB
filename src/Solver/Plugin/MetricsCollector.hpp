#ifndef PACE2026_METRICS_COLLECTOR_HPP
#define PACE2026_METRICS_COLLECTOR_HPP

#include <chrono>
#include <map>
#include <string>

namespace solver::plugin
{

/// \brief Shared metrics accumulator passed to all metric plugins.
///
/// Holds cumulative statistics gathered during a solver run. All three metric
/// plugins (\ref RuleStatsPlugin, \ref BranchCountPlugin, \ref ConvergencePlugin)
/// receive the same \c shared_ptr<MetricsCollector> so they read and write
/// consistent data.  RuleStatsPlugin and BranchCountPlugin write into it;
/// ConvergencePlugin reads consistent snapshots from it in onNewBestSolution.
struct MetricsCollector
{
    /// \brief Point in time when solving started (set by ConvergencePlugin::init).
    std::chrono::steady_clock::time_point startTime;

    /// \brief Cumulative count of applications per rule name.
    std::map<std::string, int> ruleCounts;

    /// \brief Cumulative wall-clock time (milliseconds) spent inside each rule.
    std::map<std::string, double> ruleTimes_ms;

    /// \brief Total number of branching-rule applications (search-tree internal nodes opened).
    int branchOpens = 0;

    /// \brief Total number of branches that reached a terminal state (search-tree leaves closed).
    /// Incremented on CutBranch and EndBranchWithSolutionCandidate; not on ImidateReturn.
    int branchCloses = 0;

    /// \brief Wall-clock seconds elapsed since \c startTime.
    [[nodiscard]] double elapsedSeconds() const
    {
        using namespace std::chrono;
        return duration<double>(steady_clock::now() - startTime).count();
    }
};

} // namespace solver::plugin

#endif  //PACE2026_METRICS_COLLECTOR_HPP
