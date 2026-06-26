#ifndef PACE2026_SOLVER_CONFIG_HPP
#define PACE2026_SOLVER_CONFIG_HPP

#include "BranchingSolverConfiguration.hpp"
#include "Plugin/MetricsPlugins.hpp"
#include "Plugin/VisualizationPlugin.hpp"

#include <memory>
#include <stdexcept>
#include <string>

namespace solver
{

/// \brief Top-level solver configuration with named factory presets.
///
/// \c SolverConfig sits above the individual solver configurations and provides
/// named factory methods for common deployment scenarios (competition tracks,
/// debugging, CI pipeline).  The goal is to make it impossible to accidentally
/// assemble a configuration that violates track rules — e.g. enabling SIGTERM
/// for an exact submission — while still leaving every field open for manual
/// adjustment after construction.
///
/// \par Layering
/// \c SolverConfig owns the high-level options: which competition track is
/// active, whether SIGTERM is armed, and which solver backend to use.
/// Solver-specific options live one level below in their respective
/// configuration structs:
/// \code
///   SolverConfig
///     ├── track            (Exact | Heuristic | LowerBound)
///     ├── enableSigterm    (POSIX builds only)
///     ├── solverType       (Branching | … future ILP variants …)
///     └── branchingConfig  (BranchingSolverConfiguration)
///           ├── activeRules
///           ├── boundedDepthSearch
///           └── plugins    (vector<shared_ptr<AbstractPlugin>>)
/// \endcode
///
/// \par Usage
/// \code
///   // Competition tracks
///   auto cfg = SolverConfig::exactTrack();
///   auto cfg = SolverConfig::heuristicTrack();
///
///   // CI / pipeline runs
///   auto cfg = SolverConfig::pipeline();
///
///   // Interactive debugging with DOT output
///   auto cfg = SolverConfig::debug("out/dot");
///
///   // Any field may be overridden after construction
///   cfg.branchingConfig.boundedDephtSearch = false;
/// \endcode
struct SolverConfig
{
    // -----------------------------------------------------------------------
    // Nested enums
    // -----------------------------------------------------------------------

    /// \brief Competition or evaluation track.
    enum class Track
    {
        Exact,      ///< Find the provably optimal solution (complete search).
        Heuristic,  ///< Best solution under a time limit; solver is stopped by SIGTERM.
        LowerBound, ///< Reserved — not yet implemented (see issue #55).
        Pipeline,   ///< Internal CI / stride benchmark mode; not a competition track.
    };

    /// \brief Which solver backend drives the search.
    ///
    /// ILP variants (\c EvalMaxSAT, \c UWrMaxSat, \c SCIP) are reserved for
    /// issue #55; the field exists now to avoid a breaking change later.
    enum class SolverType
    {
        Branching, ///< Branching + reduction rules solver (\ref BranchingSolver).
        Reduction, ///< Subtree Reduction (\ref ReductionSolver).
        Cluster,   ///< Cluster Reduction (\ref ClusterSolver).
    };

    // -----------------------------------------------------------------------
    // Fields (all public so callers can adjust after construction)
    // -----------------------------------------------------------------------

    /// \brief Active competition / evaluation track.  Default: \c Exact.
    Track track = Track::Exact;

    /// \brief Whether to arm a POSIX SIGTERM handler.
    ///
    /// When \c true the caller is expected to:
    ///  1. Register a signal handler that sets an \c atomic<bool> flag.
    ///  2. Push a \ref solver::plugin::SigtermPlugin (wired to the same flag)
    ///     into \c branchingConfig.plugins.
    ///  3. Call \c BranchingSolver::setTimeoutFlag with that flag.
    ///
    /// The exact track must never enable SIGTERM; callers should assert this
    /// invariant when forwarding the flag to the solver.
    ///
    /// Default: \c false.
    bool enableSigterm = false;

    /// \brief Which solver backend to use.  Default: \c Reduction -> Cluster -> Branching.
    std::vector<SolverType> solverPipeline = {SolverType::Reduction, SolverType::Cluster, SolverType::Branching};

    /// \brief Solver-specific options for the branching solver backend.
    ///
    /// When an ILP solver is eventually added (issue #55) it will get its own
    /// configuration struct alongside \c branchingConfig.  \c SolverConfig
    /// will then act as a discriminated union, selecting whichever config is
    /// active via \c solverType.
    BranchingSolverConfiguration branchingConfig;

    // -----------------------------------------------------------------------
    // Named factory presets
    // -----------------------------------------------------------------------

    /// \brief Preset for the PACE exact competition track.
    ///
    /// Produces a provably optimal solution.  SIGTERM is disabled (the exact
    /// track has a hard wall-clock limit but the solver must run to completion
    /// within it, not be interrupted).  No metrics plugins are loaded.
    ///
    /// \return A \c SolverConfig ready for the exact competition track.
    static SolverConfig exactTrack()
    {
        SolverConfig c;
        c.track = Track::Exact;
        c.enableSigterm = false;
        return c;
    }

    /// \brief Preset for the PACE heuristic competition track.
    ///
    /// The solver is allowed to be stopped at any time by a SIGTERM signal
    /// and must return the best solution found so far.
    /// Metrics plugins are opt-in — add them to \c branchingConfig.plugins after
    /// construction if needed.
    ///
    /// \note The caller is responsible for registering the OS-level signal
    ///       handler, pushing a \ref solver::plugin::SigtermPlugin, and calling
    ///       \c BranchingSolver::setTimeoutFlag when \c enableSigterm is \c true.
    ///
    /// \return A \c SolverConfig ready for the heuristic competition track.
    static SolverConfig heuristicTrack()
    {
        SolverConfig c;
        c.track = Track::Heuristic;
        c.solverPipeline = {SolverType::Reduction, SolverType::Branching};
        c.enableSigterm = true;
        return c;
    }

    /// \brief Preset for CI pipeline / stride benchmark runs.
    ///
    /// Loads the full metrics plugin suite via
    /// \ref solver::plugin::MetricsPlugins::makeAll so every pipeline run
    /// produces convergence traces, branch statistics, and rule-application
    /// counts.  SIGTERM is enabled so a time-limited run still emits a
    /// timeout stride line and the collected metrics up to the interrupt point
    /// are visible to the evaluation harness.
    ///
    /// \return A \c SolverConfig with all three metrics plugins pre-loaded and
    ///         SIGTERM armed.
    static SolverConfig pipeline(std::ostream& out = std::cout)
    {
        SolverConfig c;
        c.track = Track::Pipeline;
        c.enableSigterm = true;
        for (auto& p : solver::plugin::MetricsPlugins::makeAll(out))
            c.branchingConfig.plugins.push_back(p);
        return c;
    }

    /// \brief Preset for interactive debugging and visualisation.
    ///
    /// Loads the full metrics plugin suite (branch count, rule statistics,
    /// convergence trace) together with a \ref solver::plugin::VisualizationPlugin
    /// that writes a DOT overview and per-state DOT files to \p dirPath.
    /// SIGTERM is disabled — debug sessions run to completion.
    ///
    /// \param dirPath Directory where DOT files are written.
    /// \return A \c SolverConfig with all metrics plugins and the visualisation
    ///         plugin pre-loaded.
    static SolverConfig debug(const std::string& dirPath, std::ostream& out = std::cout)
    {
        SolverConfig c;
        for (auto& p : solver::plugin::MetricsPlugins::makeAll(out))
            c.branchingConfig.plugins.push_back(p);
        c.branchingConfig.plugins.push_back(
            std::make_shared<solver::plugin::VisualizationPlugin>(dirPath));
        return c;
    }

    /// \brief Reserved placeholder for the lower-bound competition track.
    ///
    /// No lower-bound solver exists yet.  The preset is defined now to reserve
    /// the name and avoid a breaking API change once the implementation is
    /// added (issue #55).
    ///
    /// \throws std::logic_error always.
    [[noreturn]] static SolverConfig lowerBoundTrack()
    {
        throw std::logic_error("SolverConfig::lowerBoundTrack() is not yet implemented");
    }
};

} // namespace solver

#endif  //PACE2026_SOLVER_CONFIG_HPP
