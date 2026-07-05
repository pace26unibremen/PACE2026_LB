#include <catch2/catch_test_macros.hpp>

#include "../src/Graph/Instance.hpp"
#include "../src/Solver/BranchingSolver.hpp"
#include "../src/Solver/BranchingSolverConfiguration.hpp"
#include "../src/Solver/LowerBoundCoordinatorConfig.hpp"
#include "../src/Solver/LowerBoundSolver.hpp"

#include <memory>
#include <sstream>
#include <vector>

namespace
{
// A two-tree instance with enough branching that the search needs at least one
// candidate solution before the certified early-exit can trigger.
constexpr const char* kInstance =
    "#p 2 4\n"
    "((1,2),(3,4));\n"
    "((1,3),(2,4));\n";

std::shared_ptr<graph::Instance> readInstance()
{
    std::istringstream in(kInstance);
    return graph::ReadInstance(in);
}

// Fake SAT that yields a scripted, monotonically increasing lower bound, one value per solve() call.
class ScriptedSat : public solver::IIncrementalLowerBound
{
  public:
    explicit ScriptedSat(std::vector<int> bounds) : bounds_(std::move(bounds)) {}
    void setTimeOut(double) override {}
    bool solve() override
    {
        if (idx_ + 1 < bounds_.size()) ++idx_;
        return false;  // never claims exact optimum
    }
    int getCurrentLowerBound() override { return bounds_[idx_]; }

  private:
    std::vector<int> bounds_;
    std::size_t idx_ = 0;
};
}  // namespace

TEST_CASE("coordinator certifies once the SAT lower bound raises the threshold above the incumbent",
          "[LowerBoundSolver][coordinator]")
{
    auto instance = readInstance();

    auto branchingConfig = std::make_shared<solver::BranchingSolverConfiguration>();
    branchingConfig->certifiedEarlyExit = true;
    auto branching = std::make_shared<solver::BranchingSolver>(instance, branchingConfig);

    // a=1, b=0: threshold == L. A scripted L well above any possible incumbent certifies the very
    // first candidate the branch slice finds, regardless of timing (deterministic, no wall-clock race).
    auto ctx = branching->GetContext();
    ctx->a = 1.0;
    ctx->b = 0;

    auto sat = std::make_shared<ScriptedSat>(std::vector<int>{1000, 1000, 1000, 1000});

    // SAT slice runs first so the threshold is raised before the branch ever runs.
    solver::LowerBoundCoordinatorConfig cfg = solver::LowerBoundCoordinatorConfig::defaults();
    cfg.schedule = {{solver::CoordinatorActor::Sat, 0.01},
                    {solver::CoordinatorActor::Branch, 0.05},
                    {solver::CoordinatorActor::Sat, 0.01},
                    {solver::CoordinatorActor::Branch, 0.05}};
    cfg.totalBudgetSeconds = 30.0;

    solver::LowerBoundSolver coordinator(instance, branching, sat, cfg);
    REQUIRE(coordinator.solve());

    // Certified: incumbent size <= floor(a*L)+b for the adopted L.
    REQUIRE(ctx->bestSolutionWeight <= static_cast<float>(ctx->certifiedThreshold));
    // Output forest is materialised (roots == incumbent size).
    REQUIRE(static_cast<float>(instance->at(0)->Roots().size()) == ctx->bestSolutionWeight);
}
