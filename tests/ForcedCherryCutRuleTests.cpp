#include <catch2/catch_test_macros.hpp>
#include "../src/Graph/Instance.hpp"
#include "../src/Solver/Rule/ForcedCherryCutRule.hpp"

TEST_CASE("ForcedCherryCutRule is not applicable for more than 2 forests", "[ForcedCherryCutRule]")
{
    std::istringstream nw("#p 3 3\n(1,(2,3));\n(1,(2,3));\n(1,(2,3));\n");
    auto instance = graph::ReadInstance(nw);
    auto context = std::make_shared<solver::Context>();

    auto rule = solver::ForcedCherryCutRule::isApplicable(instance, context);
    CHECK(rule == nullptr);
}

TEST_CASE("ForcedCherryCutRule is not applicable when the T1 cherry is also a T2 cherry", "[ForcedCherryCutRule]")
{
    std::istringstream nw("#p 2 3\n(1,(2,3));\n(1,(2,3));\n");
    auto instance = graph::ReadInstance(nw);
    auto context = std::make_shared<solver::Context>();

    // (2,3) is a cherry in both forests — EqualPairReductionRule's job, not this rule's.
    auto rule = solver::ForcedCherryCutRule::isApplicable(instance, context);
    CHECK(rule == nullptr);
}

TEST_CASE("ForcedCherryCutRule cuts e_a, e_b, e_c in T2 only, leaving T1 untouched", "[ForcedCherryCutRule]")
{
    // T1: (2,3) is a cherry.  T2: 2's sibling is 4, and 3 sits elsewhere — (2,3) is not a T2 cherry.
    std::istringstream nw("#p 2 4\n(1,(2,3));\n((2,4),(1,3));\n");
    auto instance = graph::ReadInstance(nw);
    auto context = std::make_shared<solver::Context>();

    auto rule = solver::ForcedCherryCutRule::isApplicable(instance, context);
    REQUIRE(rule != nullptr);
    CHECK(rule->name() == "ForcedCherryCutRule");

    auto t1RootsBefore = instance->at(0)->Roots().size();
    rule->apply();

    // T1 (instance->at(0)) must be untouched by this rule.
    CHECK(instance->at(0)->Roots().size() == t1RootsBefore);

    // T2 (instance->at(1)) cuts edges to 2, 4, 3, creating 4 separate roots.
    CHECK(instance->at(1)->Roots().size() == 4);

    rule->unapply();
    CHECK(instance->at(1)->Roots().size() == 1);
}

TEST_CASE("ForcedCherryCutRule handles aNode's T2 parent being a root with two children", "[ForcedCherryCutRule]")
{
    // T1: (2,3) is a cherry. T2: 2's parent is the root itself, with exactly two children
    // (2 and the subtree (4,(1,3))). Cutting e_a promotes 2's sibling to a root as a side
    // effect of DeleteEdgeAction::doParentIsRoot(), which must not crash the subsequent cut.
    //
    // aLabel/cLabel are passed explicitly (bypassing isApplicable's unordered_map-driven
    // cherry search, whose iteration order is not guaranteed to pick a=2) to deterministically
    // reproduce the topology that triggers the doParentIsRoot() side effect on aNode's cut.
    std::istringstream nw("#p 2 4\n(1,(2,3));\n(2,(4,(1,3)));\n");
    auto instance = graph::ReadInstance(nw);
    auto context = std::make_shared<solver::Context>();

    auto rule = std::make_shared<solver::ForcedCherryCutRule>(instance, context, /*aLabel=*/2, /*cLabel=*/3);

    auto t2RootsBefore = instance->at(1)->Roots().size();
    CHECK_NOTHROW(rule->apply());

    // Cutting should increase the number of roots in T2 without throwing.
    CHECK(instance->at(1)->Roots().size() > t2RootsBefore);

    CHECK_NOTHROW(rule->unapply());
    CHECK(instance->at(1)->Roots().size() == t2RootsBefore);
}
