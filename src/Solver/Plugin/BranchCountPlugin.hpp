#ifndef PACE2026_BRANCH_COUNT_PLUGIN_HPP
#define PACE2026_BRANCH_COUNT_PLUGIN_HPP

#include "AbstractStridePlugin.hpp"
#include "MetricsCollector.hpp"

#include <memory>

namespace solver::plugin
{

/// \brief Plugin that counts branching-rule applications and terminal branches, and emits both totals.
///
/// On \c onEnd() emits:
/// \code
///   #s branch_opens N
///   #s branch_closes M
/// \endcode
class BranchCountPlugin : public AbstractStridePlugin
{
    std::shared_ptr<MetricsCollector> collector;

  public:
    explicit BranchCountPlugin(std::shared_ptr<MetricsCollector> collector, std::ostream& out = std::cout);

    /// \brief Increments \c collector->branchOpens when \p rule is an \ref AbstractBranchingRule.
    void onApply(const std::shared_ptr<solver::AbstractRule>& rule) override;

    /// \brief Increments \c collector->branchCloses when a branch reaches a terminal state.
    void onBranchEnd() override;

    void onEnd() override;
};

} // namespace solver::plugin

#endif  //PACE2026_BRANCH_COUNT_PLUGIN_HPP
