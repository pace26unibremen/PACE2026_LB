#include <catch2/catch_test_macros.hpp>

#include "../src/Graph/Instance.hpp"
#include "../src/Solver/BranchingSolver.hpp"
#include "../src/Solver/Plugin/AbstractPlugin.hpp"
#include "../src/Solver/Plugin/VisualizationPlugin.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

// ---------------------------------------------------------------------------
// MockPlugin — records the name of every hook that was called, in order.
// ---------------------------------------------------------------------------
class MockPlugin : public solver::plugin::AbstractPlugin
{
  public:
    std::vector<std::string> calls;

    void init(const std::shared_ptr<graph::Instance>&,
              const std::shared_ptr<solver::Context>&) override             { calls.push_back("init"); }
    void beforeApply(const std::shared_ptr<solver::AbstractRule>&) override { calls.push_back("beforeApply"); }
    void onApply(const std::shared_ptr<solver::AbstractRule>&) override     { calls.push_back("onApply"); }
    void onUnapply(const std::shared_ptr<solver::AbstractRule>&) override   { calls.push_back("onUnapply"); }
    void onReductionUnapply(const std::shared_ptr<solver::AbstractRule>&, bool) override { calls.push_back("onReductionUnapply"); }
    void onReductionReapply(const std::shared_ptr<solver::AbstractRule>&) override       { calls.push_back("onReductionReapply"); }
    void onNewBestSolution(float) override                            { calls.push_back("onNewBestSolution"); }
    void onBranchEnd() override                                                  { calls.push_back("onBranchEnd"); }
    void onEnd() override                                                   { calls.push_back("onEnd"); }

    int count(const std::string& name) const
    {
        return static_cast<int>(std::count(calls.begin(), calls.end(), name));
    }
};

// ---------------------------------------------------------------------------
// Helper: build a solver config with the given plugins and depth-search mode.
// ---------------------------------------------------------------------------
static std::shared_ptr<solver::BranchingSolverConfiguration> makeConfig(
    std::vector<std::shared_ptr<solver::plugin::AbstractPlugin>> plugins,
    bool bounded)
{
    auto config = std::make_shared<solver::BranchingSolverConfiguration>();
    config->plugins = std::move(plugins);
    config->boundedDephtSearch = bounded;
    return config;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

TEST_CASE("Plugin: init is called first and onEnd is called last", "[Plugin]")
{
    SECTION("bounded depth search")
    {
        auto mock = std::make_shared<MockPlugin>();
        auto instance = graph::ReadInstance(std::string(RES_DIR) + "tiny/tiny01.nw");
        solver::BranchingSolver(instance, makeConfig({mock}, true)).solve();

        REQUIRE_FALSE(mock->calls.empty());
        CHECK(mock->calls.front() == "init");
        CHECK(mock->calls.back() == "onEnd");
    }

    SECTION("unbounded depth search")
    {
        auto mock = std::make_shared<MockPlugin>();
        auto instance = graph::ReadInstance(std::string(RES_DIR) + "tiny/tiny01.nw");
        solver::BranchingSolver(instance, makeConfig({mock}, false)).solve();

        REQUIRE_FALSE(mock->calls.empty());
        CHECK(mock->calls.front() == "init");
        CHECK(mock->calls.back() == "onEnd");
    }
}

TEST_CASE("Plugin: init and onEnd are each called exactly once", "[Plugin]")
{
    SECTION("bounded depth search")
    {
        auto mock = std::make_shared<MockPlugin>();
        auto instance = graph::ReadInstance(std::string(RES_DIR) + "tiny/tiny01.nw");
        solver::BranchingSolver(instance, makeConfig({mock}, true)).solve();

        CHECK(mock->count("init") == 1);
        CHECK(mock->count("onEnd") == 1);
    }

    SECTION("unbounded depth search")
    {
        auto mock = std::make_shared<MockPlugin>();
        auto instance = graph::ReadInstance(std::string(RES_DIR) + "tiny/tiny01.nw");
        solver::BranchingSolver(instance, makeConfig({mock}, false)).solve();

        CHECK(mock->count("init") == 1);
        CHECK(mock->count("onEnd") == 1);
    }
}

TEST_CASE("Plugin: beforeApply is immediately followed by onApply", "[Plugin]")
{
    auto mock = std::make_shared<MockPlugin>();
    auto instance = graph::ReadInstance(std::string(RES_DIR) + "tiny/tiny01.nw");
    solver::BranchingSolver(instance, makeConfig({mock}, true)).solve();

    // Every onApply must be directly preceded by a beforeApply.
    std::string prev;
    for (const auto& call : mock->calls)
    {
        if (call == "onApply")
        {
            CHECK(prev == "beforeApply");
        }
        prev = call;
    }

    // There must be at least as many beforeApply calls as onApply calls.
    CHECK(mock->count("beforeApply") == mock->count("onApply"));
}

TEST_CASE("Plugin: multiple plugins all receive the same sequence of hooks", "[Plugin]")
{
    auto mock1 = std::make_shared<MockPlugin>();
    auto mock2 = std::make_shared<MockPlugin>();
    auto instance = graph::ReadInstance(std::string(RES_DIR) + "tiny/tiny01.nw");
    solver::BranchingSolver(instance, makeConfig({mock1, mock2}, true)).solve();

    CHECK(mock1->calls == mock2->calls);
}

TEST_CASE("Plugin: onNewBestSolution is called at least once in unbounded mode", "[Plugin]")
{
    // In unbounded mode the solver explores all branches and calls
    // onNewBestSolution each time it improves the incumbent solution.
    auto mock = std::make_shared<MockPlugin>();
    auto instance = graph::ReadInstance(std::string(RES_DIR) + "tiny/tiny01.nw");
    solver::BranchingSolver(instance, makeConfig({mock}, false)).solve();

    CHECK(mock->count("onNewBestSolution") >= 1);
}

TEST_CASE("Plugin: onBranchEnd is called at least once in both modes", "[Plugin]")
{
    SECTION("bounded depth search")
    {
        auto mock = std::make_shared<MockPlugin>();
        auto instance = graph::ReadInstance(std::string(RES_DIR) + "tiny/tiny01.nw");
        solver::BranchingSolver(instance, makeConfig({mock}, true)).solve();

        CHECK(mock->count("onBranchEnd") >= 1);
    }

    SECTION("unbounded depth search")
    {
        auto mock = std::make_shared<MockPlugin>();
        auto instance = graph::ReadInstance(std::string(RES_DIR) + "tiny/tiny01.nw");
        solver::BranchingSolver(instance, makeConfig({mock}, false)).solve();

        CHECK(mock->count("onBranchEnd") >= 1);
    }
}

TEST_CASE("Plugin: onBranchEnd always precedes onEnd", "[Plugin]")
{
    // onBranchEnd must always fire before onEnd — it represents the final
    // branch reaching a terminal state.
    SECTION("bounded depth search — single leaf then onEnd")
    {
        auto mock = std::make_shared<MockPlugin>();
        auto instance = graph::ReadInstance(std::string(RES_DIR) + "tiny/tiny01.nw");
        solver::BranchingSolver(instance, makeConfig({mock}, true)).solve();

        // In bounded mode the last two events must be onBranchEnd then onEnd.
        REQUIRE(mock->calls.size() >= 2);
        const auto n = mock->calls.size();
        CHECK(mock->calls[n - 2] == "onBranchEnd");
        CHECK(mock->calls[n - 1] == "onEnd");
    }
}

TEST_CASE("VisualizationPlugin: overview.dot is a syntactically closed DOT graph", "[Plugin][VisualizationPlugin]")
{
    auto tempDir = std::filesystem::temp_directory_path() / "pace2026_viz_test";
    std::filesystem::remove_all(tempDir);

    {
        auto plugin = std::make_shared<solver::plugin::VisualizationPlugin>(tempDir.string());
        auto instance = graph::ReadInstance(std::string(RES_DIR) + "tiny/tiny01.nw");
        solver::BranchingSolver(instance, makeConfig({plugin}, true)).solve();
    }

    auto overviewPath = tempDir / "overview.dot";
    REQUIRE(std::filesystem::exists(overviewPath));

    std::ifstream file(overviewPath);
    REQUIRE(file.is_open());
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    // File must open with the expected DOT header.
    CHECK(content.rfind("digraph overview", 0) == 0);

    // File must end with a closing brace (the only valid DOT terminator).
    auto lastNonSpace = content.find_last_not_of(" \t\n\r");
    REQUIRE(lastNonSpace != std::string::npos);
    CHECK(content[lastNonSpace] == '}');

    std::filesystem::remove_all(tempDir);
}
