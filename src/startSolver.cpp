#if defined(USE_INCR)
#include "Solver/ILP/IncrementalMAFSolver.hpp"
#elif defined(USE_EVALMAXSAT) || defined(USE_SCIP) || defined(USE_UWRMAXSAT)
#include "Solver/ILP/MAFILPSolver.hpp"
#else
#include "Solver/BranchingSolver.hpp"
#endif

#include "Solver/Cluster/ClusterSolver.hpp"
#include "Solver/Cluster/ClusterRange.hpp"
#include "Solver/Plugin/SigtermPlugin.hpp"
#include "Solver/ReductionSolver.hpp"
#include "Solver/SolverConfig.hpp"

#include <atomic>
#include <cassert>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

// ============================================================
//  Default preset
//
//  Change the return value to switch tracks without touching
//  anything else.  To go beyond what a preset offers, build
//  and adjust a config here instead:
//
//    auto c = solver::SolverConfig::exactTrack();
//    c.branchingConfig.boundedDephtSearch = false;
//    c.branchingConfig.plugins.push_back(...);
//    return c;
// ============================================================
static solver::SolverConfig defaultConfig(std::ostream& /*out*/)
{
    // exactTrack has no plugins — out is unused here but kept for a uniform
    // signature with resolveConfig so callers don't need to special-case it.
    return solver::SolverConfig::exactTrack();
}

// ============================================================
//  POSIX SIGTERM infrastructure
//  Written by the OS signal handler; read by the solver loop.
// ============================================================
#ifdef _POSIX_VERSION
static std::atomic<bool> g_timeout{false};
static void sigtermHandler(int) { g_timeout.store(true, std::memory_order_relaxed); }
#endif

// ============================================================
//  Core solver loop
// ============================================================

/// \brief Solve one instance read from \p in and write the result to \p out.
///
/// \param in      Input stream in .nw / .tree format.
/// \param out     Output stream for the solution and timing header.
/// \param config  Fully constructed \ref solver::SolverConfig.
///                SIGTERM arming (signal registration + plugin wiring) is
///                handled here based on \c config.enableSigterm.
static void runOnStream(std::istream& in, std::ostream& out, solver::SolverConfig config)
{
    // Safety invariant: exact track must never be SIGTERM-armed.
    assert(!(config.track == solver::SolverConfig::Track::Exact && config.enableSigterm)
           && "Exact track must never enable SIGTERM");

    const auto startTime = std::clock();
    const auto instance = graph::ReadInstance(in);

    bool solved = false;

    // all solvers that worked on the solution
    std::vector<std::shared_ptr<solver::AbstractSolver>> solverList;
    // stores the cluster solver (and the access to the clusters) that may be part of the pipeline
    solver::ClusterSolver* clusterSolver = nullptr;

    for (auto solverType : config.solverPipeline)
    {
        switch (solverType)
        {
            case solver::SolverConfig::SolverType::Branching:
            {

#ifdef _POSIX_VERSION
                if (config.enableSigterm) {
                    std::signal(SIGTERM, sigtermHandler);
                    config.branchingConfig.plugins.push_back(
                        std::make_shared<solver::plugin::SigtermPlugin>(&g_timeout, out));
                }
#endif
                if (clusterSolver)
                {
                    bool allClustersSolved = true;
                    for (const auto& [cluster, context] : clusterSolver->Clusters())
                    {
                        auto branchingConfig = std::make_shared<solver::BranchingSolverConfiguration>(config.branchingConfig);
                        auto solver = std::make_shared<solver::BranchingSolver>(cluster, branchingConfig, context);
#ifdef   _POSIX_VERSION
                        if (config.enableSigterm)
                        {
                            solver->setTimeoutFlag(&g_timeout);
                        }
#endif
                        solverList.push_back(solver);
                        allClustersSolved &= solver->solve();
                    }
                    solved = allClustersSolved;
                }
                else
                {
                    auto branchingConfig = std::make_shared<solver::BranchingSolverConfiguration>(config.branchingConfig);
                    auto solver = std::make_shared<solver::BranchingSolver>(instance, branchingConfig);
#ifdef   _POSIX_VERSION
                    if (config.enableSigterm)
                    {
                        solver->setTimeoutFlag(&g_timeout);
                    }
#endif
                    solverList.push_back(solver);
                    solved = solver->solve();
                }
                break;
            }
            case solver::SolverConfig::SolverType::Reduction:
            {
                auto solver = std::make_shared<solver::ReductionSolver>(instance);
                solverList.push_back(solver);
                solver->solve();
                break;
            }
            case solver::SolverConfig::SolverType::Cluster:
            {
                auto solver = std::make_shared<solver::ClusterSolver>(instance);
                solverList.push_back(solver);
                solver->solve();
                clusterSolver = solver.get();
                break;
            }
            default:
            {
                std::clog << "Solver pipeline contains unknown solver\n";
                return;
            }
        }
    }

    for (int i = solverList.size()-1; i >= 0; --i)
    {
        solverList[i]->unapplyReductions();
    }

    if (!solved) {
        // Only reachable when SIGTERM fires before the very first solution
        // candidate — i.e. within the first few milliseconds of a run on an
        // instance where even reaching a leaf takes longer than the grace
        // period.  In normal heuristic-track usage the solver keeps searching
        // until at least one candidate is found, so this path should be rare.
        std::clog << "Solver stopped without producing a solution\n";
        return;
    }

    const auto endTime = std::clock();
    const double time_delta_ms = static_cast<double>(endTime - startTime)
                                 / (static_cast<double>(CLOCKS_PER_SEC) / 1000.0);

    out << "# t " << time_delta_ms << "\n"
        << "# s " << instance->at(0)->Roots().size() << "\n";
    instance->at(0)->write(out);
}

// ============================================================
//  CLI helpers
// ============================================================

/// \brief Look up \c PACE_TRACK in a \c .env file in the current working directory.
///
/// Follows the common dotenv convention:
///  - Lines starting with \c # are comments.
///  - Blank lines are ignored.
///  - The first \c PACE_TRACK=VALUE match wins.
///  - Leading and trailing whitespace around key and value is stripped.
///
/// Returns the value string if found, or \c std::nullopt otherwise.
/// Does not modify the process environment.
static std::optional<std::string> readPaceTrackFromDotEnv()
{
    std::ifstream f(".env");
    if (!f) return std::nullopt;

    std::string line;
    while (std::getline(f, line)) {
        // strip trailing whitespace / CR
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r'))
            line.pop_back();

        if (line.empty() || line.front() == '#') continue;

        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        // strip whitespace from key
        std::string_view k{line.data(), eq};
        while (!k.empty() && (k.front() == ' ' || k.front() == '\t')) k.remove_prefix(1);
        while (!k.empty() && (k.back()  == ' ' || k.back()  == '\t')) k.remove_suffix(1);

        if (k != "PACE_TRACK") continue;

        // strip leading whitespace from value
        std::string_view v{line.data() + eq + 1, line.size() - eq - 1};
        while (!v.empty() && (v.front() == ' ' || v.front() == '\t')) v.remove_prefix(1);

        return std::string(v);
    }
    return std::nullopt;
}

static void printHelp(std::string_view prog)
{
    std::cout
        << "Usage: " << prog << " [--track PRESET] [INFILE [OUTFILE]]\n"
        << "       " << prog << " --help\n"
        << "\n"
        << "Solve the Maximum Agreement Forest (MAF) problem.\n"
        << "\n"
        << "Positional arguments:\n"
        << "  INFILE   Input forest file (.nw / .tree). Omit to read from stdin.\n"
        << "  OUTFILE  Output file. Defaults to INFILE + \"_solution.tree\".\n"
        << "           Omitting INFILE implies stdout for output as well.\n"
        << "\n"
        << "Options:\n"
        << "  --track PRESET  Solver preset to use (default: pipeline):\n"
        << "    pipeline    Full metrics suite, SIGTERM armed.\n"
        << "                Use for CI runs and the PACE stride harness.\n"
        << "    exact       No plugins, no SIGTERM. Use for the exact competition track.\n"
        << "    heuristic   No metrics, SIGTERM armed. Use for the heuristic track.\n"
        << "  --help, -h  Print this message and exit.\n"
        << "\n"
        << "Stdin / stdout mode:\n"
        << "  Invoking with no file arguments reads from stdin and writes to stdout.\n"
        << "  This is the mode used by optil.io and the PACE stride evaluation harness.\n"
        << "  Passing a single empty string as the only argument is treated identically\n"
        << "  (some harnesses inject this).\n"
        << "\n"
        << "Track selection precedence (highest to lowest):\n"
        << "  1. --track flag on the command line\n"
        << "  2. PACE_TRACK environment variable\n"
        << "  3. PACE_TRACK in a .env file in the current working directory\n"
        << "  4. defaultConfig() in startSolver.cpp  (compiled-in default)\n"
        << "\n"
        << "  Accepted values for PACE_TRACK: pipeline, exact, heuristic\n";
}

/// \brief Map a track name string to a \ref solver::SolverConfig.
///
/// \param out  Stream that plugin output (stride lines) will be written to.
///             Should be the same stream the solution is written to so that
///             both solution data and metrics land in the same destination.
/// \throws std::invalid_argument for unknown names.
static solver::SolverConfig resolveConfig(const std::string& name, std::ostream& out)
{
    if (name == "exact")     return solver::SolverConfig::exactTrack();
    if (name == "heuristic") return solver::SolverConfig::heuristicTrack();
    if (name == "pipeline")  return solver::SolverConfig::pipeline(out);
    throw std::invalid_argument(
        "Unknown --track value: \"" + name + "\". "
        "Valid values: exact, heuristic, pipeline.");
}

// ============================================================
//  Entry point
// ============================================================
int main(int argc, char* argv[])
{
    std::string trackName;
    std::string infile;
    std::string outfile;

    // ---- Parse arguments ------------------------------------------------
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};

        if (arg == "--help" || arg == "-h") {
            printHelp(argv[0]);
            return 0;
        }

        if (arg == "--track") {
            if (++i >= argc)
                throw std::invalid_argument("--track requires an argument");
            trackName = argv[i];
            continue;
        }

        if (arg.starts_with("--track=")) {
            trackName = std::string(arg.substr(8));
            continue;
        }

        if (arg.starts_with("--")) {
            throw std::invalid_argument("Unknown option: " + std::string(arg)
                                        + ". Run with --help for usage.");
        }

        // Empty string: some evaluation harnesses (e.g. optil.io) pass a
        // single empty argument; treat it as if no file argument were given.
        if (arg.empty()) continue;

        if (infile.empty())  { infile  = std::string(arg); continue; }
        if (outfile.empty()) { outfile = std::string(arg); continue; }
        throw std::invalid_argument("Too many positional arguments. Run with --help for usage.");
    }

    // ---- Resolve preset: CLI flag > env var > .env file > compiled default
    if (trackName.empty()) {
        if (const char* env = std::getenv("PACE_TRACK"); env != nullptr)
            trackName = env;
    }
    if (trackName.empty()) {
        if (auto dotenv = readPaceTrackFromDotEnv())
            trackName = std::move(*dotenv);
    }

    // ---- Open streams first so plugin output follows solution output -----
    //
    // Plugins (metrics, SIGTERM) write to the same stream as the solution so
    // that both land in the same file or on stdout together.  The config must
    // therefore be built after the output stream is known.

    if (infile.empty()) {
        // Stdin / stdout mode — used by optil.io and the PACE stride harness.
        const solver::SolverConfig config = trackName.empty()
                                                ? defaultConfig(std::cout)
                                                : resolveConfig(trackName, std::cout);
        runOnStream(std::cin, std::cout, config);
        return 0;
    }

    if (outfile.empty())
        outfile = infile + "_solution.tree";

    std::ifstream inStream(infile);
    if (!inStream)
        throw std::runtime_error("Cannot open input file: " + infile);

    std::ofstream outStream(outfile);
    if (!outStream)
        throw std::runtime_error("Cannot open output file: " + outfile);

    const solver::SolverConfig config = trackName.empty()
                                            ? defaultConfig(outStream)
                                            : resolveConfig(trackName, outStream);
    runOnStream(inStream, outStream, config);
    return 0;
}
