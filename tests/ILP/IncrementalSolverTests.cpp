#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include "../../src/Graph/Instance.hpp"
#include "../../src/Solver/ILP/IncrementalMAFSolver.hpp"

// Expected number of roots in the MAF solution for each tiny instance.
// These match the expected values from BranchingSolverSimpleTests.
std::unordered_map<std::string, unsigned int> instanceToSolutionSize =
{
    {"tiny01.nw", 4},
    {"tiny04.nw", 5},
    {"tiny05.nw", 3},
    {"tiny06.nw", 3},
    {"tiny07.nw", 8},
    {"tiny08.nw", 12},
    {"tiny10.nw", 6},
};

TEST_CASE("IncrementalMAFSolver with UWrMaxSAT on Tiny Test Set", "[IncrementalMAFSolver, UWrMaxSAT, Tiny]")
{
    for (const std::string& f : {"tiny01.nw", "tiny04.nw", "tiny05.nw",
                                  "tiny06.nw", "tiny07.nw", "tiny08.nw", "tiny10.nw"})
    {
        SECTION("solve " + f)
        {
            auto instance = graph::ReadInstance(std::string(ILP_TEST_DIR) + f);
            auto solver = solver::IncrementalMAFSolver(instance, solver::MaxSATSolverType::UWrMaxSAT);
            auto solved = solver.solve();

            CHECK(solved);
            CHECK(solver.Instance()->at(0)->Roots().size() == instanceToSolutionSize[f]);
        }
    }
}