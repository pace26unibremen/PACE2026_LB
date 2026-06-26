#include <catch2/catch_test_macros.hpp>

#include "../src/Solver/Plugin/BranchCountPlugin.hpp"
#include "../src/Solver/Plugin/ConvergencePlugin.hpp"
#include "../src/Solver/Plugin/MetricsCollector.hpp"
#include "../src/Solver/Plugin/MetricsPlugins.hpp"
#include "../src/Solver/Plugin/RuleStatsPlugin.hpp"
#include "../src/Solver/Plugin/SigtermPlugin.hpp"
#include "../src/Solver/Rule/AbstractBranchingRule.hpp"
#include "../src/Solver/Rule/RuleReturnCode.hpp"

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

// ---------------------------------------------------------------------------
// Helpers: expose protected static helpers via a thin subclass
// ---------------------------------------------------------------------------

class StrideTestHelper : public solver::plugin::AbstractStridePlugin
{
  public:
    using AbstractStridePlugin::AbstractStridePlugin; // inherit stream constructor
    using AbstractStridePlugin::emitStrideLine;
    using AbstractStridePlugin::toJson;
    using AbstractStridePlugin::toJsonSnakeKeys;
    using AbstractStridePlugin::toSnakeCase;
};

// ---------------------------------------------------------------------------
// Mock rules (null instance/context — we never call apply/unapply)
// ---------------------------------------------------------------------------

class MockReductionRule : public solver::AbstractRule
{
    std::string ruleName;

  public:
    explicit MockReductionRule(std::string name)
        : AbstractRule(nullptr, nullptr, /*isReduction=*/true), ruleName(std::move(name))
    {
    }

    solver::RuleReturnCode apply() override { return solver::RuleReturnCode::Continue; }
    void unapply() override {}
    [[nodiscard]] std::string name() const override { return ruleName; }
    std::shared_ptr<AbstractRule> clone() const override { return std::make_shared<MockReductionRule>(name()); }
};

class MockBranchingRule : public solver::AbstractBranchingRule
{
    std::string ruleName;

  public:
    explicit MockBranchingRule(std::string name)
        : AbstractBranchingRule(nullptr, nullptr, /*maxBranch=*/2), ruleName(std::move(name))
    {
    }

    solver::RuleReturnCode apply() override { return solver::RuleReturnCode::Continue; }
    void unapply() override {}
    [[nodiscard]] std::string name() const override { return ruleName; }
    std::shared_ptr<AbstractRule> clone() const override { return std::make_shared<MockBranchingRule>(name()); }
};

// ---------------------------------------------------------------------------
// Utility: capture stdout while running a functor
// ---------------------------------------------------------------------------

template<typename F>
static std::string captureStdout(F&& fn)
{
    std::ostringstream buf;
    auto* old = std::cout.rdbuf(buf.rdbuf());
    fn();
    std::cout.rdbuf(old);
    return buf.str();
}

// ===========================================================================
// AbstractStridePlugin: toJson helpers
// ===========================================================================

TEST_CASE("AbstractStridePlugin: toJson(map<string,int>)", "[MetricsPlugin][JSON]")
{
    SECTION("empty map")
    {
        CHECK(StrideTestHelper::toJson(std::map<std::string, int>{}) == "{}");
    }

    SECTION("single entry")
    {
        CHECK(StrideTestHelper::toJson(std::map<std::string, int>{{"CutBranchRule", 3}}) == "{\"CutBranchRule\":3}");
    }

    SECTION("multiple entries are alphabetically sorted (map order)")
    {
        std::map<std::string, int> m{{"b_rule", 2}, {"a_rule", 5}};
        // std::map iterates in key order, so a_rule comes first
        CHECK(StrideTestHelper::toJson(m) == "{\"a_rule\":5,\"b_rule\":2}");
    }
}

TEST_CASE("AbstractStridePlugin: toJson(map<string,double>)", "[MetricsPlugin][JSON]")
{
    SECTION("empty map")
    {
        CHECK(StrideTestHelper::toJson(std::map<std::string, double>{}) == "{}");
    }

    SECTION("single entry — three decimal places")
    {
        std::map<std::string, double> m{{"r", 1.5}};
        CHECK(StrideTestHelper::toJson(m) == "{\"r\":1.500}");
    }

    SECTION("value zero")
    {
        std::map<std::string, double> m{{"r", 0.0}};
        CHECK(StrideTestHelper::toJson(m) == "{\"r\":0.000}");
    }
}

TEST_CASE("AbstractStridePlugin: toJson(vector<Snapshot>)", "[MetricsPlugin][JSON]")
{
    SECTION("empty vector")
    {
        CHECK(StrideTestHelper::toJson(std::vector<solver::plugin::Snapshot>{}) == "[]");
    }

    SECTION("single snapshot — field order and format, rule keys converted to snake_case")
    {
        solver::plugin::Snapshot s{
            .wtime        = 0.123,
            .weight       = 4,
            .ruleCounts   = {{"CutBranchRule", 2}},
            .branchOpens  = 1,
            .branchCloses = 1,
            .ruleTimes_ms = {{"CutBranchRule", 7.5}},
        };
        const std::string json = StrideTestHelper::toJson(std::vector<solver::plugin::Snapshot>{s});
        // Rule name keys are converted to snake_case by toJson(Snapshot) at serialisation time.
        CHECK(json == "[{\"wtime\":0.123,\"score\":4.0,\"branch_opens\":1,\"branch_closes\":1,"
                      "\"rule_counts\":{\"cut_branch_rule\":2},"
                      "\"rule_times_ms\":{\"cut_branch_rule\":7.500}}]");
    }

    SECTION("two snapshots are comma-separated")
    {
        solver::plugin::Snapshot s1{.wtime = 0.1, .weight = 5, .ruleCounts = {}, .branchOpens = 0, .branchCloses = 0, .ruleTimes_ms = {}};
        solver::plugin::Snapshot s2{.wtime = 0.2, .weight = 3, .ruleCounts = {}, .branchOpens = 1, .branchCloses = 1, .ruleTimes_ms = {}};
        const std::string json = StrideTestHelper::toJson(std::vector<solver::plugin::Snapshot>{s1, s2});
        CHECK(json.front() == '[');
        CHECK(json.back() == ']');
        // There must be exactly one comma separating the two objects at top level.
        const auto first_close = json.find('}');
        REQUIRE(first_close != std::string::npos);
        CHECK(json[first_close + 1] == ',');
    }
}

TEST_CASE("AbstractStridePlugin: toSnakeCase converts CamelCase to snake_case", "[MetricsPlugin][JSON]")
{
    CHECK(StrideTestHelper::toSnakeCase("CutBranchRule")          == "cut_branch_rule");
    CHECK(StrideTestHelper::toSnakeCase("PairPathBranchingRule")  == "pair_path_branching_rule");
    CHECK(StrideTestHelper::toSnakeCase("EqualForestsRule")       == "equal_forests_rule");
    CHECK(StrideTestHelper::toSnakeCase("SingleVertexTreePropagationRule") == "single_vertex_tree_propagation_rule");
    CHECK(StrideTestHelper::toSnakeCase("already_snake")          == "already_snake");
    CHECK(StrideTestHelper::toSnakeCase("R")                      == "r");
}

TEST_CASE("AbstractStridePlugin: emitStrideLine writes #s prefix", "[MetricsPlugin][stride]")
{
    std::ostringstream ss;
    StrideTestHelper helper{ss};
    helper.emitStrideLine("branch_opens", "42");
    CHECK(ss.str() == "#s branch_opens 42\n");
}

// ===========================================================================
// RuleStatsPlugin
// ===========================================================================

TEST_CASE("RuleStatsPlugin: accumulates counts and emits stride lines on onEnd", "[MetricsPlugin][RuleStats]")
{
    auto collector = std::make_shared<solver::plugin::MetricsCollector>();
    solver::plugin::RuleStatsPlugin plugin(collector);

    auto ruleA = std::make_shared<MockReductionRule>("RuleA");
    auto ruleB = std::make_shared<MockReductionRule>("RuleB");

    // Simulate: RuleA applied twice, RuleB applied once.
    plugin.beforeApply(ruleA); plugin.onApply(ruleA);
    plugin.beforeApply(ruleA); plugin.onApply(ruleA);
    plugin.beforeApply(ruleB); plugin.onApply(ruleB);

    // Collector stores CamelCase keys (rule->name()); conversion happens at emit time only.
    REQUIRE(collector->ruleCounts.at("RuleA") == 2);
    REQUIRE(collector->ruleCounts.at("RuleB") == 1);

    const std::string out = captureStdout([&] { plugin.onEnd(); });

    // Both stride keys must appear.
    CHECK(out.find("#s rule_times_ms ") != std::string::npos);
    CHECK(out.find("#s rule_counts ") != std::string::npos);

    // rule_counts JSON must encode the right numbers with snake_case keys.
    CHECK(out.find("\"rule_a\":2") != std::string::npos);
    CHECK(out.find("\"rule_b\":1") != std::string::npos);
}

TEST_CASE("RuleStatsPlugin: timing is non-negative", "[MetricsPlugin][RuleStats]")
{
    auto collector = std::make_shared<solver::plugin::MetricsCollector>();
    solver::plugin::RuleStatsPlugin plugin(collector);
    auto rule = std::make_shared<MockReductionRule>("R");

    plugin.beforeApply(rule);
    plugin.onApply(rule);

    CHECK(collector->ruleTimes_ms.at("R") >= 0.0);
}

TEST_CASE("RuleStatsPlugin: collectTiming=false still counts rules but skips timing", "[MetricsPlugin][RuleStats]")
{
    auto collector = std::make_shared<solver::plugin::MetricsCollector>();
    solver::plugin::RuleStatsPlugin plugin(collector, /*collectTiming=*/false);

    auto ruleA = std::make_shared<MockReductionRule>("RuleA");
    auto ruleB = std::make_shared<MockReductionRule>("RuleB");

    plugin.beforeApply(ruleA); plugin.onApply(ruleA);
    plugin.beforeApply(ruleA); plugin.onApply(ruleA);
    plugin.beforeApply(ruleB); plugin.onApply(ruleB);

    // Counts must still be accumulated (CamelCase keys in the collector).
    CHECK(collector->ruleCounts.at("RuleA") == 2);
    CHECK(collector->ruleCounts.at("RuleB") == 1);

    // Timing map must remain empty.
    CHECK(collector->ruleTimes_ms.empty());
}

TEST_CASE("RuleStatsPlugin: collectTiming=false emits empty rule_times_ms on onEnd", "[MetricsPlugin][RuleStats]")
{
    auto collector = std::make_shared<solver::plugin::MetricsCollector>();
    solver::plugin::RuleStatsPlugin plugin(collector, /*collectTiming=*/false);

    auto rule = std::make_shared<MockReductionRule>("R");
    plugin.beforeApply(rule); plugin.onApply(rule);

    const std::string out = captureStdout([&] { plugin.onEnd(); });
    CHECK(out.find("#s rule_times_ms {}") != std::string::npos);
    CHECK(out.find("#s rule_counts ") != std::string::npos);
}

// ===========================================================================
// BranchCountPlugin
// ===========================================================================

TEST_CASE("BranchCountPlugin: counts only branching rules", "[MetricsPlugin][BranchCount]")
{
    auto collector = std::make_shared<solver::plugin::MetricsCollector>();
    solver::plugin::BranchCountPlugin plugin(collector);

    auto reduction = std::make_shared<MockReductionRule>("Reduction");
    auto branch    = std::make_shared<MockBranchingRule>("Branch");

    plugin.onApply(reduction);
    plugin.onApply(reduction);
    plugin.onApply(branch);
    plugin.onApply(branch);
    plugin.onApply(branch);

    CHECK(collector->branchOpens == 3);
}

TEST_CASE("BranchCountPlugin: onBranchEnd increments branchCloses", "[MetricsPlugin][BranchCount]")
{
    auto collector = std::make_shared<solver::plugin::MetricsCollector>();
    solver::plugin::BranchCountPlugin plugin(collector);

    plugin.onBranchEnd();
    plugin.onBranchEnd();
    plugin.onBranchEnd();

    CHECK(collector->branchCloses == 3);
}

TEST_CASE("BranchCountPlugin: onEnd emits #s branch_opens and #s branch_closes", "[MetricsPlugin][BranchCount]")
{
    auto collector = std::make_shared<solver::plugin::MetricsCollector>();
    solver::plugin::BranchCountPlugin plugin(collector);

    auto branch = std::make_shared<MockBranchingRule>("B");
    plugin.onApply(branch);
    plugin.onApply(branch);
    plugin.onBranchEnd();

    const std::string out = captureStdout([&] { plugin.onEnd(); });
    CHECK(out == "#s branch_opens 2\n#s branch_closes 1\n");
}

TEST_CASE("BranchCountPlugin: zero branches emits 0 for both keys", "[MetricsPlugin][BranchCount]")
{
    auto collector = std::make_shared<solver::plugin::MetricsCollector>();
    solver::plugin::BranchCountPlugin plugin(collector);

    const std::string out = captureStdout([&] { plugin.onEnd(); });
    CHECK(out == "#s branch_opens 0\n#s branch_closes 0\n");
}

// ===========================================================================
// ConvergencePlugin
// ===========================================================================

TEST_CASE("ConvergencePlugin: onEnd with no improvements emits empty array", "[MetricsPlugin][Convergence]")
{
    auto collector = std::make_shared<solver::plugin::MetricsCollector>();
    solver::plugin::ConvergencePlugin plugin(collector);

    plugin.init(nullptr, nullptr);

    const std::string out = captureStdout([&] { plugin.onEnd(); });
    CHECK(out == "#s convergence []\n");
}

TEST_CASE("ConvergencePlugin: snapshots are cumulative and chronological", "[MetricsPlugin][Convergence]")
{
    auto collector = std::make_shared<solver::plugin::MetricsCollector>();

    // Pre-populate the collector as if RuleStatsPlugin and BranchCountPlugin
    // had already processed some rules before the first improvement.
    collector->ruleCounts["R"] = 3;
    collector->branchOpens     = 1;
    collector->branchCloses    = 1;

    solver::plugin::ConvergencePlugin plugin(collector);
    plugin.init(nullptr, nullptr);

    // First improvement
    plugin.onNewBestSolution(5);

    // Simulate more rule applications before the second improvement.
    collector->ruleCounts["R"] = 6;
    collector->branchOpens     = 3;
    collector->branchCloses    = 2;

    // Second improvement
    plugin.onNewBestSolution(3);

    const std::string out = captureStdout([&] { plugin.onEnd(); });

    // The line must start with the stride prefix.
    CHECK(out.substr(0, 16) == "#s convergence [");

    // First snapshot has score 5, branch_opens 1, branch_closes 1.
    CHECK(out.find("\"score\":5") != std::string::npos);
    CHECK(out.find("\"branch_opens\":1") != std::string::npos);
    CHECK(out.find("\"branch_closes\":1") != std::string::npos);

    // Second snapshot has score 3, branch_opens 3, branch_closes 2.
    CHECK(out.find("\"score\":3") != std::string::npos);
    CHECK(out.find("\"branch_opens\":3") != std::string::npos);
    CHECK(out.find("\"branch_closes\":2") != std::string::npos);

    // Two snapshots → exactly two score fields.
    std::size_t count = 0;
    std::size_t pos   = 0;
    while ((pos = out.find("\"score\":", pos)) != std::string::npos) { ++count; ++pos; }
    CHECK(count == 2);
}

// ===========================================================================
// MetricsPlugins factory
// ===========================================================================

TEST_CASE("MetricsPlugins::makeAll returns three plugins sharing one collector",
          "[MetricsPlugin][Factory]")
{
    const auto plugins = solver::plugin::MetricsPlugins::makeAll();
    REQUIRE(plugins.size() == 3);
    for (const auto& p : plugins)
    {
        REQUIRE(p != nullptr);
    }
}

TEST_CASE("MetricsPlugins::makeAll: shared collector — RuleStats data visible to Convergence",
          "[MetricsPlugin][Factory]")
{
    // Build the suite.
    auto plugins = solver::plugin::MetricsPlugins::makeAll();
    // plugins[0] = RuleStatsPlugin, plugins[2] = ConvergencePlugin

    auto ruleStats   = std::dynamic_pointer_cast<solver::plugin::RuleStatsPlugin>(plugins[0]);
    auto convergence = std::dynamic_pointer_cast<solver::plugin::ConvergencePlugin>(plugins[2]);
    REQUIRE(ruleStats   != nullptr);
    REQUIRE(convergence != nullptr);

    // init starts the collector clock (via ConvergencePlugin).
    convergence->init(nullptr, nullptr);

    // Fire a rule through RuleStatsPlugin.
    auto rule = std::make_shared<MockReductionRule>("TestRule");
    ruleStats->beforeApply(rule);
    ruleStats->onApply(rule);

    // Signal an improvement through ConvergencePlugin.
    convergence->onNewBestSolution(4);

    // The snapshot should contain the rule count accumulated by RuleStatsPlugin.
    const std::string out = captureStdout([&] { convergence->onEnd(); });
    CHECK(out.find("\"test_rule\":1") != std::string::npos);
}

// ===========================================================================
// SigtermPlugin
// ===========================================================================

TEST_CASE("SigtermPlugin: emits timeout 0 when flag is not set", "[MetricsPlugin][Sigterm]")
{
    std::atomic<bool> flag{false};
    solver::plugin::SigtermPlugin plugin(&flag);

    const std::string out = captureStdout([&] { plugin.onEnd(); });
    CHECK(out == "#s timeout 0\n");
}

TEST_CASE("SigtermPlugin: emits timeout 1 when flag is set", "[MetricsPlugin][Sigterm]")
{
    std::atomic<bool> flag{true};
    solver::plugin::SigtermPlugin plugin(&flag);

    const std::string out = captureStdout([&] { plugin.onEnd(); });
    CHECK(out == "#s timeout 1\n");
}

TEST_CASE("SigtermPlugin: emits timeout 0 when constructed with nullptr", "[MetricsPlugin][Sigterm]")
{
    solver::plugin::SigtermPlugin plugin(nullptr);

    const std::string out = captureStdout([&] { plugin.onEnd(); });
    CHECK(out == "#s timeout 0\n");
}
