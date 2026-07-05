#include <catch2/catch_test_macros.hpp>

#include "../src/Graph/Instance.hpp"
#include "../src/Solver/BranchingSolver.hpp"
#include "../src/Solver/BranchingSolverConfiguration.hpp"
#include "../src/Solver/Context.hpp"
#include "../src/Solver/ILP/IncrementalMAFSolver.hpp"

#include <chrono>
#include <memory>
#include <sstream>

using namespace std::chrono;

// Regression test for the C1 coordinator bug: the SAT solver (IncrementalMAFSolver) must never be
// handed the SAME live instance the BranchingSolver mutates in place. A pause slice returns without
// unwinding (by design), so if the SAT and the branch shared one Instance, the SAT's live-forest reads
// (getNumVars/getRootIndex/TerminalToLabel) would race against maps snapshotted from the pristine
// forest at construction time -> unsound lower bound or a crash on a dangling Node*.
//
// The fix (src/startSolver.cpp) constructs the SAT over an independent deep copy of the instance
// (graph::Forest::copy()) taken while the shared instance is still pristine. This test reproduces that
// setup directly: it constructs the SAT over a copy, then destructively mutates the ORIGINAL shared
// instance via a branch slice with an already-expired pause deadline (guaranteed to apply >= 1 mutating
// rule before pausing without unwinding -- see BranchingSolver::advanceSearch's madeProgressThisCall
// guard), and asserts the SAT still yields a valid (<=  known optimum) lower bound and does not crash.
namespace
{
// A two-tree instance with enough branching that a single already-expired-deadline advanceSearch()
// call is guaranteed to apply at least one mutating rule and then pause (see
// BranchingSolverPauseResumeTests.cpp, which uses the same instance for the same reason).
constexpr const char* kInstance =
    "#p 2 4\n"
    "((1,2),(3,4));\n"
    "((1,3),(2,4));\n";

std::shared_ptr<graph::Instance> readInstance()
{
    std::istringstream in(kInstance);
    return graph::ReadInstance(in);
}

// Mirrors the fix in startSolver.cpp: a fresh Instance built from deep copies of the original's
// forests, so mutating the original can never reach the copy.
std::shared_ptr<graph::Instance> deepCopy(const std::shared_ptr<graph::Instance>& original)
{
    auto copy = std::make_shared<graph::Instance>();
    copy->reserve(original->size());
    for (const auto& forest : *original)
        copy->push_back(std::make_shared<graph::Forest>(forest->copy()));
    return copy;
}
}  // namespace

TEST_CASE("IncrementalMAFSolver isolated on its own instance copy survives a paused branch's "
          "in-place mutation of the shared instance",
          "[IncrementalMAFSolver][isolation][regression][C1]")
{
    // Known optimum: an uninterrupted reference solve of the branching solver on a pristine instance.
    auto refInstance = readInstance();
    solver::BranchingSolver reference(refInstance);
    REQUIRE(reference.solve());
    const int knownOptimum = static_cast<int>(reference.GetContext()->bestSolutionWeight);

    // The shared instance, exactly as startSolver.cpp hands one to both the branching solver and (via
    // the fix) a COPY of it to the SAT.
    auto instance = readInstance();

    // Fix under test: take the SAT's copy now, while `instance` is still pristine.
    auto satInstance = deepCopy(instance);
    auto context = std::make_shared<solver::Context>();
    solver::IncrementalMAFSolver sat(satInstance, context);

    // Destructively mutate the ORIGINAL, shared instance: a branch slice with an already-expired pause
    // deadline applies at least one mutating rule/branch step and then pauses WITHOUT unwinding --
    // exactly what LowerBoundSolver's schedule does before running a SAT slice.
    auto branchingConfig = std::make_shared<solver::BranchingSolverConfiguration>();
    solver::BranchingSolver branching(instance, branchingConfig);
    branching.setPauseDeadline(steady_clock::now());  // already expired -> pause ASAP
    const auto result = branching.advanceSearch();
    REQUIRE(result == solver::BranchingSolver::RunResult::Paused);

    // The SAT must be unaffected by the branch's in-place mutation of `instance`: it still operates on
    // its own independent copy, so solving it must not crash and must yield a lower bound that never
    // exceeds the true optimum.
    sat.setTimeOut(10.0);
    CHECK_NOTHROW(sat.solve());
    CHECK(sat.getCurrentLowerBound() <= knownOptimum);
}
