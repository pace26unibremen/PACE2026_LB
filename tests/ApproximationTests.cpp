#include <catch2/catch_test_macros.hpp>

#include "../src/Graph/Instance.hpp"
#include "../src/Solver/BranchingSolver.hpp"
#include "../src/Solver/BranchingSolverConfiguration.hpp"

#include <memory>
#include <string>
#include <unordered_map>

// Runs the approximation (approximationRules, any number of trees) on `inst` in place and returns the
// size (component count) of the constructed agreement forest — mirroring how the solver seeds its
// incumbent (see startSolver's seedFromApproximation). No ReductionSolver pre-pass is needed: the
// approximation rules are complete on their own (reduction is only a batching speed-up).
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

static const std::unordered_map<std::string, unsigned int> optimum =
{
    {"tiny01.nw", 4}, {"tiny02.nw", 1}, {"tiny03.nw", 7}, {"tiny04.nw", 5}, {"tiny05.nw", 3},
    {"tiny06.nw", 3}, {"tiny07.nw", 8}, {"tiny08.nw", 12}, {"tiny09.nw", 5}, {"tiny10.nw", 6},
};

TEST_CASE("approximation builds a valid forest; two-tree instances stay within the proven 3x ratio",
          "[Approximation][Tiny]")
{
    for (const std::string& f : {"tiny01.nw", "tiny02.nw", "tiny03.nw", "tiny04.nw", "tiny05.nw",
                                 "tiny06.nw", "tiny07.nw", "tiny08.nw", "tiny09.nw", "tiny10.nw"})
    {
        SECTION("approximate " + f)
        {
            auto instance = graph::ReadInstance(std::string(RES_DIR) + "tiny/" + f);
            const unsigned int opt = optimum.at(f);

            const unsigned int size = approximate(instance);

            // The constructed forest is always a valid agreement forest, so its size is a valid upper
            // bound on the optimal MAF size for any number of trees.
            CHECK(size >= opt);

            // For two trees, the Whidden-Zeh 3-approximation guarantees e_approx <= 3 * e_opt, where
            // e = MAF size - 1 is the rSPR distance.
            if (instance->size() == 2)
            {
                CHECK(size - 1 <= 3 * (opt - 1));
            }
        }
    }
}
