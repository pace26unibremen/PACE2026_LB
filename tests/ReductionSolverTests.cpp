#include <catch2/catch_test_macros.hpp>

#include <sstream>

#include "../src/Graph/Instance.hpp"
#include "../src/Solver/ReductionSolver.hpp"
#include "../src/Solver/Rule/ChainReductionRule.hpp"
#include "../src/Solver/Rule/SubtreeReductionRule.hpp"

// This instance is deliberately built so that ChainReductionRule cannot fire AT ALL until
// SubtreeReductionRule has collapsed a cherry first:
//
//   forest0: (((( 5,(3,(4,9)) ),(1,2)),6),7),(8,10))
//   forest1: (((( 5,(3,4)     ),(1,2)),6),7),(8,(9,10)))
//
// "5"'s immediate sibling is the cherry (1,2) ("N1"). ChainReductionRule's uncle-walk needs
// N1 to be a *terminal* to treat it as the next chain element (see ChainReductionRule.cpp:
// "the uncle must be a terminal" check) - but a raw, uncollapsed cherry is an internal node,
// not a terminal, so the walk from "5" cannot even get started. Only after
// SubtreeReductionRule collapses (1,2) into a single terminal (label 1) does the chain
// 5 -> 1 -> 6 -> 7 become walkable and long enough (length 4, exceeding the
// solver's reduced-chain-length threshold of forests+1 = 3) to be shortened by
// ChainReductionRule.
//
// The two forests intentionally differ in how "3","4","9" are arranged below "5" (a filler
// subtree, present purely so "5" itself is not itself a sibling-pair chain-start) so that
// SubtreeReductionRule's own single climb stops exactly at the (1,2) cherry and does not
// cascade further up on its own - this way the terminal-count reduction actually observed can
// only be explained by SubtreeReductionRule and ChainReductionRule genuinely cooperating,
// not by SubtreeReductionRule alone (unlike the old tiny02-based test, where
// SubtreeReductionRule alone already collapsed everything and ChainReductionRule never
// contributed anything - see task-2-report.md Critical-1).
static const std::string kNw =
    "#p 2 10\n"
    "(((((5,(3,(4,9))),(1,2)),6),7),(8,10));\n"
    "(((((5,(3,4)),(1,2)),6),7),(8,(9,10)));\n";

TEST_CASE("ReductionSolver: ChainReductionRule is blocked until SubtreeReductionRule collapses a cherry first",
          "[ReductionSolver]")
{
    // Sanity check: SubtreeReductionRule alone (run repeatedly to its own fixpoint, exactly as
    // it already does internally in one call) never gets past collapsing the (1,2) cherry -
    // it cannot reach the reduction that ChainReductionRule performs.
    {
        std::istringstream in(kNw);
        auto instance = graph::ReadInstance(in);
        auto context = std::make_shared<solver::Context>();
        while (auto rule = solver::SubtreeReductionRule::isApplicable(instance, context))
        {
            rule->apply();
        }
        CHECK(instance->at(0)->TerminalToLabel().size() == 9);
    }

    // Sanity check: ChainReductionRule alone can never fire at all on the untouched instance,
    // because the (1,2) cherry is not yet a terminal, so the chain-walk from "5" can't even
    // begin.
    {
        std::istringstream in(kNw);
        auto instance = graph::ReadInstance(in);
        auto context = std::make_shared<solver::Context>();
        auto rule = solver::ChainReductionRule::isApplicable(instance, context);
        CHECK(rule == nullptr);
    }
}

TEST_CASE("ReductionSolver: solve() lets SubtreeReductionRule and ChainReductionRule cooperate, and "
          "unapplyReductions() restores the original instance",
          "[ReductionSolver]")
{
    std::istringstream in(kNw);
    auto instance = graph::ReadInstance(in);

    REQUIRE(instance->size() == 2);
    REQUIRE(instance->at(0)->TerminalToLabel().size() == 10);

    auto solver = solver::ReductionSolver(instance);
    solver.solve();

    // SubtreeReductionRule alone can only ever get to 9 terminals (see sanity check above).
    // Reaching fewer than that here proves ChainReductionRule actually fired, and that it did
    // so using the result of SubtreeReductionRule's collapse - i.e. the two rules genuinely
    // cooperated within solve().
    CHECK(instance->at(0)->TerminalToLabel().size() < 9);
    CHECK(instance->at(1)->TerminalToLabel().size() < 9);

    solver.unapplyReductions();

    // After unapply, the original (unreduced) structure is fully restored.
    CHECK(instance->at(0)->TerminalToLabel().size() == 10);
    CHECK(instance->at(1)->TerminalToLabel().size() == 10);
    for (unsigned int label = 1; label <= 10; label++)
    {
        CHECK(instance->at(0)->LabelToTerminal().contains(label));
        CHECK(instance->at(1)->LabelToTerminal().contains(label));
    }
}
