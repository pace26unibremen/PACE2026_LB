#include <catch2/catch_test_macros.hpp>

#include "../src/Graph/Instance.hpp"
#include "../src/Solver/BranchingSolver.hpp"
#include "../src/Solver/BranchingSolverConfiguration.hpp"
#include "../src/Solver/Context.hpp"
#include "../src/Solver/DualLowerBound.hpp"

#include <cmath>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

// Exact optima k* for the tiny instances (same values used by ApproximationTests).
static const std::unordered_map<std::string, long> kStar = {
    {"tiny01.nw", 4}, {"tiny02.nw", 1}, {"tiny03.nw", 7}, {"tiny04.nw", 5}, {"tiny05.nw", 3},
    {"tiny06.nw", 3}, {"tiny07.nw", 8}, {"tiny08.nw", 12}, {"tiny09.nw", 5}, {"tiny10.nw", 6},
};

// Pinned certified lower bounds L produced by the current C++ port. These are a regression guard:
// they must always stay <= the matching k* above (checked separately), and any change here signals
// the dual bookkeeping shifted and must be re-validated with the L <= k* batch harness.
static const std::unordered_map<std::string, long> pinnedL = {
    {"tiny01.nw", 2}, {"tiny02.nw", 1}, {"tiny03.nw", 1}, {"tiny04.nw", 3}, {"tiny05.nw", 2},
    {"tiny06.nw", 2}, {"tiny07.nw", 5}, {"tiny08.nw", 7}, {"tiny09.nw", 1}, {"tiny10.nw", 1},
};

static std::shared_ptr<graph::Instance> readTiny(const std::string& f)
{
    return graph::ReadInstance(std::string(RES_DIR) + "tiny/" + f);
}

TEST_CASE("dual lower bound never exceeds k* and is at least 1", "[DualLowerBound][Tiny]")
{
    for (const auto& [f, ks] : kStar)
    {
        SECTION(f)
        {
            const long L = solver::computeDual3ApproxLowerBound(*readTiny(f));
            // The non-negotiable invariant: L is a certified lower bound on the optimum.
            CHECK(L >= 1);
            CHECK(L <= ks);
        }
    }
}

TEST_CASE("dual lower bound matches its pinned regression values", "[DualLowerBound][Tiny]")
{
    for (const auto& [f, expected] : pinnedL)
    {
        SECTION(f)
        {
            CHECK(solver::computeDual3ApproxLowerBound(*readTiny(f)) == expected);
        }
    }
}

TEST_CASE("certifiedCeiling computes floor(a*L)+b without binary-float under-count", "[DualLowerBound][Context]")
{
    solver::Context ctx;
    ctx.b = 0;

    SECTION("a = 1.15, L = 20 -> exactly 23 (a naive floor of the runtime double gives 22)")
    {
        ctx.a = 1.15;
        // 1.15 * 20 == 23 exactly, but 1.15 is not representable in binary: a runtime (double)1.15 * 20
        // evaluates to 22.999… and a bare std::floor gives 22. certifiedCeiling's 1e-9 nudge must give 23.
        CHECK(ctx.certifiedCeiling(20) == 23);
        volatile double aRuntime = 1.15;
        volatile long Lruntime = 20;
        const long naive = static_cast<long>(std::floor(aRuntime * static_cast<double>(Lruntime)));
        CHECK(naive <= 23);  // may be 22 (the pitfall we correct) or 23; never above
    }

    SECTION("offset b is added after the floor")
    {
        ctx.a = 1.2;
        ctx.b = 5;
        CHECK(ctx.certifiedCeiling(3) == 8);   // floor(1.2*3) + 5 = 3 + 5
        CHECK(ctx.certifiedCeiling(10) == 17); // floor(1.2*10) + 5 = 12 + 5
    }

    SECTION("a = 1.0 is the identity times L, plus b")
    {
        ctx.a = 1.0;
        ctx.b = 4;
        CHECK(ctx.certifiedCeiling(7) == 11);
    }
}

TEST_CASE("ReadInstance parses '#a {a} {b}' into the context", "[DualLowerBound][Parsing]")
{
    // Build a stream: an "#a" line followed by a real tiny instance body.
    std::ifstream body(std::string(RES_DIR) + "tiny/tiny04.nw");
    std::stringstream ss;
    ss << "#a 1.15 7\n" << body.rdbuf();

    auto ctx = std::make_shared<solver::Context>();
    auto instance = graph::ReadInstance(ss, ctx);

    REQUIRE(instance->size() == 2);
    CHECK(ctx->a == 1.15);
    CHECK(ctx->b == 7);
    // And the ceiling flows through: floor(1.15*20)+7 = 23+7.
    CHECK(ctx->certifiedCeiling(20) == 30);
}

TEST_CASE("ReadInstance without an '#a' line leaves the lower-bound fields unset", "[DualLowerBound][Parsing]")
{
    auto ctx = std::make_shared<solver::Context>();
    graph::ReadInstance(std::string(RES_DIR) + "tiny/tiny04.nw", ctx);
    CHECK(ctx->a == -1.0);
    CHECK(ctx->b == -1);
    CHECK(ctx->certifiedThreshold == -1);
}

// Seed a branching solver from the approximation exactly like startSolver::seedFromApproximation.
static std::pair<std::list<std::shared_ptr<solver::AbstractRule>>, unsigned int>
seedFromApproximation(const std::shared_ptr<graph::Instance>& inst)
{
    auto approxConfig = std::make_shared<solver::BranchingSolverConfiguration>(
        solver::BranchingSolverConfiguration::approximationRules());
    auto approxSolver = std::make_shared<solver::BranchingSolver>(inst, approxConfig);
    approxSolver->solve();
    const unsigned int size = static_cast<unsigned int>(inst->at(0)->Roots().size());
    auto branch = approxSolver->SolutionBranch();
    approxSolver->unapplySolutionBranch();
    return {std::move(branch), size};
}

TEST_CASE("branching solver certified early-exit stops at the threshold with a valid answer",
          "[DualLowerBound][EarlyExit]")
{
    // tiny04: k* = 5, approximation seed = 7.
    const long ks = 5;

    SECTION("threshold >= seed: emit the seed immediately, without searching to the optimum")
    {
        auto instance = readTiny("tiny04.nw");
        auto [branch, seed] = seedFromApproximation(instance);
        REQUIRE(seed == 7);

        auto ctx = std::make_shared<solver::Context>();
        ctx->certifiedThreshold = 8;  // >= seed 7

        auto config = std::make_shared<solver::BranchingSolverConfiguration>();
        config->certifiedEarlyExit = true;
        solver::BranchingSolver s(instance, config, ctx);
        s.seedSolution(std::move(branch), static_cast<float>(seed));
        REQUIRE(s.solve());

        const long emitted = static_cast<long>(instance->at(0)->Roots().size());
        CHECK(emitted == 7);              // stopped at the seed, did not search down to 5
        CHECK(emitted <= ks + 3);         // and it is a valid answer for a=1.0,b=3 (ceiling 8)
    }

    SECTION("threshold between seed and optimum: stop as soon as an incumbent certifies")
    {
        auto instance = readTiny("tiny04.nw");
        auto [branch, seed] = seedFromApproximation(instance);

        auto ctx = std::make_shared<solver::Context>();
        ctx->certifiedThreshold = 6;  // between k*=5 and seed=7

        auto config = std::make_shared<solver::BranchingSolverConfiguration>();
        config->certifiedEarlyExit = true;
        solver::BranchingSolver s(instance, config, ctx);
        s.seedSolution(std::move(branch), static_cast<float>(seed));
        REQUIRE(s.solve());

        const long emitted = static_cast<long>(instance->at(0)->Roots().size());
        CHECK(emitted <= 6);   // certified: within threshold
        CHECK(emitted >= ks);  // never below the true optimum
    }

    SECTION("no certified context: the search still runs to the optimum (unchanged behaviour)")
    {
        auto instance = readTiny("tiny04.nw");
        auto [branch, seed] = seedFromApproximation(instance);

        auto config = std::make_shared<solver::BranchingSolverConfiguration>();
        solver::BranchingSolver s(instance, config);  // default context, early exit disabled
        s.seedSolution(std::move(branch), static_cast<float>(seed));
        REQUIRE(s.solve());

        const long emitted = static_cast<long>(instance->at(0)->Roots().size());
        CHECK(emitted == ks);  // full search finds the optimum 5
    }
}
