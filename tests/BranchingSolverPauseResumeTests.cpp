#include <catch2/catch_test_macros.hpp>

#include "../src/Graph/Instance.hpp"
#include "../src/Solver/BranchingSolver.hpp"
#include "../src/Solver/BranchingSolverConfiguration.hpp"

#include <chrono>
#include <sstream>

using namespace std::chrono;

namespace
{
// A two-tree instance with enough branching that the search takes many
// iterations, so a zero-length pause deadline is guaranteed to interrupt it
// mid-search at least once.
constexpr const char* kInstance =
    "#p 2 4\n"
    "((1,2),(3,4));\n"
    "((1,3),(2,4));\n";

std::shared_ptr<graph::Instance> readInstance()
{
    std::istringstream in(kInstance);
    return graph::ReadInstance(in);
}
}  // namespace

TEST_CASE("paused-and-resumed search yields the same optimum as an uninterrupted run",
          "[BranchingSolver][pause]")
{
    // Reference: uninterrupted solve.
    auto refInstance = readInstance();
    solver::BranchingSolver reference(refInstance);
    REQUIRE(reference.solve());
    const auto referenceWeight = reference.GetContext()->bestSolutionWeight;

    // Paused run: advance with an already-expired pause deadline repeatedly until Solved/Exhausted.
    auto pausedInstance = readInstance();
    solver::BranchingSolver paused(pausedInstance);
    solver::BranchingSolver::RunResult result;
    int guard = 0;
    do
    {
        paused.setPauseDeadline(steady_clock::now());  // already expired -> pause ASAP
        result = paused.advanceSearch();
        REQUIRE(++guard < 100000);
    } while (result == solver::BranchingSolver::RunResult::Paused);

    REQUIRE((result == solver::BranchingSolver::RunResult::Solved
             || result == solver::BranchingSolver::RunResult::Exhausted));
    paused.finalize();
    REQUIRE(paused.GetContext()->bestSolutionWeight == referenceWeight);
}
