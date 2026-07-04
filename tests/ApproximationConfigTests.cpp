#include <catch2/catch_test_macros.hpp>
#include "../src/Graph/Instance.hpp"
#include "../src/Solver/BranchingSolver.hpp"

TEST_CASE("approximationRules preset solves a tiny disagreeing instance without branching", "[ApproximationConfig]")
{
    // T1 has cherry (2,3); T2 does not (2's T2 sibling is 4) — forces exactly one
    // ForcedCherryCutRule application, after which everything resolves via the free rules.
    // T1: ((1,4),(2,3)) - cherry (2,3), with (1,4) as separate subtree
    // T2: ((2,4),(1,3)) - no cherry (2,3), 2's sibling is 4 instead
    std::istringstream nw("#p 2 4\n((1,4),(2,3));\n((2,4),(1,3));\n");
    auto instance = graph::ReadInstance(nw);

    auto config = std::make_shared<solver::BranchingSolverConfiguration>(
        solver::BranchingSolverConfiguration::approximationRules());
    auto solver = solver::BranchingSolver(instance, config);

    bool solved = solver.solve();
    CHECK(solved);

    solver.unapplyReductions();
    // Collapse-style reductions are undone (label count back to the original 4),
    // but the ForcedCherryCutRule cut itself remains applied — see the precision
    // note in Task 5.
    CHECK(instance->at(0)->TerminalToLabel().size() == 4);
}
