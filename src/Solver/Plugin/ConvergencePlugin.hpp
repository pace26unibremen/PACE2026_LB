#ifndef PACE2026_CONVERGENCE_PLUGIN_HPP
#define PACE2026_CONVERGENCE_PLUGIN_HPP

#include "AbstractStridePlugin.hpp"
#include "MetricsCollector.hpp"

#include <memory>
#include <vector>

namespace solver::plugin
{

/// \brief Plugin that records a convergence trace and emits it as a JSON array.
///
/// \c init() resets the collector's start clock.  Each call to
/// \c onNewBestSolution() appends a \ref Snapshot holding the wall time, score,
/// and cumulative rule statistics at that improvement point.  On \c onEnd()
/// the full trace is emitted as one stride line:
/// \code
///   #s convergence [{...},{...},...]
/// \endcode
///
/// The line is written before the MAF solution because \c onEnd() is invoked
/// before \c *instance = {solution} in the solver loop.
///
/// Array length is bounded by \c initial_upper_bound - \c final_score, which
/// is typically below 50 elements per instance.
class ConvergencePlugin : public AbstractStridePlugin
{
    std::shared_ptr<MetricsCollector> collector;
    std::vector<Snapshot> snapshots;

  public:
    explicit ConvergencePlugin(std::shared_ptr<MetricsCollector> collector, std::ostream& out = std::cout);

    /// \brief Records \c collector->startTime = steady_clock::now().
    void init(const std::shared_ptr<graph::Instance>& instance,
              const std::shared_ptr<solver::Context>& context) override;

    /// \brief Appends a snapshot of the current collector state to the trace.
    void onNewBestSolution(float weight) override;

    /// \brief Emits `#s convergence [...]` to \ref out_.
    void onEnd() override;
};

} // namespace solver::plugin

#endif  //PACE2026_CONVERGENCE_PLUGIN_HPP
