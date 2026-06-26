#ifndef PACE2026_VISUALIZATION_PLUGIN_HPP
#define PACE2026_VISUALIZATION_PLUGIN_HPP

#include "AbstractPlugin.hpp"

#include <fstream>
#include <stack>

namespace solver::plugin
{

/// \brief Plugin that visualizes the solver's search space as a DOT graph.
///
/// Records each rule application and instance state, producing an interactive
/// overview file and per-state DOT files in a given output directory.
/// The resulting files can be rendered with Graphviz (e.g. via \c dot -Tsvg).
class VisualizationPlugin : public AbstractPlugin
{
    /// \brief IDs assigned to states on the current branch
    std::stack<int> stateIDs = std::stack<int>();

    /// \brief highest state ID assigned so far
    int maxStateId = 0;

    /// \brief directory where DOT files are written
    std::string dirPath;

    /// \brief output stream for the overview DOT file
    std::ofstream overviewFile;

    /// \brief snapshot of all forests at solve-start (to mark forests removed by reductions)
    std::shared_ptr<graph::Instance> shadowInstance;

    void writeStateNode();
    void writeRuleNode(const std::shared_ptr<solver::AbstractRule>& rule);
    void dotInstance(const std::filesystem::path& path);

  public:
    /// \param dirPath path to the directory where DOT files are written
    explicit VisualizationPlugin(std::string dirPath);

    void init(const std::shared_ptr<graph::Instance>& instance,
              const std::shared_ptr<solver::Context>& context) override;
    void onApply(const std::shared_ptr<solver::AbstractRule>& rule) override;
    void onUnapply(const std::shared_ptr<solver::AbstractRule>& rule) override;
    void onReductionUnapply(const std::shared_ptr<solver::AbstractRule>& rule, bool lastRule) override;
    void onReductionReapply(const std::shared_ptr<solver::AbstractRule>& rule) override;
    void onEnd() override;
};

} // namespace solver::plugin

#endif  //PACE2026_VISUALIZATION_PLUGIN_HPP
