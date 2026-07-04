#include <catch2/catch_test_macros.hpp>

#include "../src/Graph/Instance.hpp"
#include "../src/Solver/BranchingSolver.hpp"
#include "../src/Solver/BranchingSolverConfiguration.hpp"
#include "../src/Solver/Rule/GreedyCherryCutRule.hpp"

#include <memory>
#include <sstream>
#include <string>
#include <utility>

// Runs the approximation (approximationRules) on `inst` in place and returns the constructed
// agreement forest's size (component count).
static unsigned int approximate(const std::shared_ptr<graph::Instance>& inst)
{
    auto config = std::make_shared<solver::BranchingSolverConfiguration>(
        solver::BranchingSolverConfiguration::approximationRules());
    solver::BranchingSolver branchingSolver(inst, config);
    branchingSolver.solve();
    // Undo the collapse-style reductions so the forest is back in the original leaf labels (the
    // solution's own cuts remain); the component count is unchanged.
    branchingSolver.unapplyReductions();
    return static_cast<unsigned int>(inst->at(0)->Roots().size());
}

TEST_CASE("GreedyCherryCutRule fires on a >2-tree conflicting cherry and isolates a leaf", "[GreedyCherryCutRule]")
{
    // f0 and f1 share the cherry (2,3); f2 breaks it ((1,2) is the cherry there), so (2,3) is not a
    // common cherry -> the greedy rule applies.
    std::istringstream nw("#p 3 3\n(1,(2,3));\n(1,(2,3));\n((1,2),3);\n");
    auto instance = graph::ReadInstance(nw);
    auto context = std::make_shared<solver::Context>();

    auto rule = solver::GreedyCherryCutRule::isApplicable(instance, context);
    REQUIRE(rule != nullptr);
    CHECK(rule->name() == "GreedyCherryCutRule");

    unsigned int rootsBefore = 0;
    for (const auto& f : *instance) rootsBefore += f->Roots().size();

    rule->apply();
    unsigned int rootsAfter = 0;
    for (const auto& f : *instance) rootsAfter += f->Roots().size();
    // One leaf is isolated in every forest in which it still had a parent -> total roots increase.
    CHECK(rootsAfter > rootsBefore);

    rule->unapply();
    unsigned int rootsRestored = 0;
    for (const auto& f : *instance) rootsRestored += f->Roots().size();
    CHECK(rootsRestored == rootsBefore);
}

TEST_CASE("GreedyCherryCutRule does not fire on a fully common cherry", "[GreedyCherryCutRule]")
{
    // (2,3) is a cherry in all three forests -> EqualPairReductionRule's job, not the greedy's.
    std::istringstream nw("#p 3 3\n(1,(2,3));\n(1,(2,3));\n(1,(2,3));\n");
    auto instance = graph::ReadInstance(nw);
    auto context = std::make_shared<solver::Context>();

    CHECK(solver::GreedyCherryCutRule::isApplicable(instance, context) == nullptr);
}

TEST_CASE("computeApproximation builds a valid agreement forest on >2-tree instances", "[GreedyCherryCutRule][Runner]")
{
    for (const auto& [name, optimum] : {std::pair<std::string, unsigned int>{"tiny03.nw", 7},
                                        std::pair<std::string, unsigned int>{"tiny09.nw", 5}})
    {
        SECTION(name)
        {
            auto instance = graph::ReadInstance(std::string(RES_DIR) + "tiny/" + name);
            REQUIRE(instance->size() > 2);
            const auto labelsBefore = instance->at(0)->TerminalToLabel().size();

            const unsigned int size = approximate(instance);

            // The constructed forest is a valid upper bound on the optimum ...
            CHECK(size >= optimum);
            // ... and covers every original leaf label.
            CHECK(instance->at(0)->TerminalToLabel().size() == labelsBefore);
        }
    }
}
