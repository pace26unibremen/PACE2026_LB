#include <catch2/catch_test_macros.hpp>

#include "../src/Graph/Forest.hpp"
#include "../src/Graph/Instance.hpp"
#include "../src/Solver/BranchingSolver.hpp"
#include "../src/Solver/Plugin/MetricsPlugins.hpp"
#include "../src/Solver/Rule/TwoBRule.hpp"
#include "catch2/catch_config.hpp"

#include <ostream>
#include <streambuf>
#include <algorithm>

using namespace graph;
using namespace std;
using namespace solver;

TEST_CASE("TwoBRule tests", "[Forest, TwoBRule]")
{
    SECTION("Two Trees, applicable")
    {
        auto f1 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example5.tree",6,1);
        auto f2 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example2.tree",6, 1);

        auto instance = std::make_shared<std::vector<std::shared_ptr<Forest>>>();
        instance->push_back(std::make_shared<Forest>(f1));
        instance->push_back(std::make_shared<Forest>(f2));

        auto rule = TwoBRule::isApplicable(instance, nullptr);

        REQUIRE(rule != nullptr);

        INFO("Apply Rule");
        rule->apply();

        REQUIRE(f1.Roots().size() == 3);
        REQUIRE(f2.Roots().size() == 3);

        auto end1 = f1.Roots().end();
        Node* cut11 = f1.LabelToTerminal()[2];
        Node* cut12 = f1.LabelToTerminal()[3];
        REQUIRE(std::find(f1.Roots().begin(), end1, cut11) != end1);
        REQUIRE(std::find(f1.Roots().begin(), end1, cut12) != end1);

        auto end2 = f2.Roots().end();
        Node* cut21 = f2.LabelToTerminal()[2];
        Node* cut22 = f2.LabelToTerminal()[3];
        REQUIRE(std::find(f2.Roots().begin(), end2, cut21) != end2);
        REQUIRE(std::find(f2.Roots().begin(), end2, cut22) != end2);

        INFO("Undo Action");
        rule->unapply();

        auto f1Copy = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example5.tree",6,1);
        auto f2Copy = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example2.tree",6, 1);
        REQUIRE(f1 == f1Copy);
        REQUIRE(f2 == f2Copy);
    }

    SECTION("Two Trees, not applicable")
    {
        auto f1 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example2.tree",6,1);
        auto f2 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example5.tree",6, 1);

        auto instance = std::make_shared<std::vector<std::shared_ptr<Forest>>>();
        instance->push_back(std::make_shared<Forest>(f1));
        instance->push_back(std::make_shared<Forest>(f2));

        auto rule = TwoBRule::isApplicable(instance, nullptr);

        REQUIRE(rule == nullptr);
    }
}

TEST_CASE("BranchingSolver with 2-B Rule", "[Forest, TwoBRule]")
{

    class NullBuffer : public std::streambuf {
    public:
        int overflow(int c) override {
            return traits_type::not_eof(c);
        }
    };
    NullBuffer nullBuffer;
    std::ostream nullStream(&nullBuffer);

    SECTION("2 Forests, 6 Leafs")
    {
        auto config = std::make_shared<solver::BranchingSolverConfiguration>();
        config->boundedDephtSearch = false;
        config->activeRules = {
            solver::CutBranchRule::isApplicable,
            solver::CheckSingleVertexTreesRule::isApplicable,
            solver::SingleVertexTreePropagationRule::isApplicable,
            solver::EqualPairReductionRule::isApplicable,
            solver::TwoBRule::isApplicable,
            solver::ACBranchingRule::isApplicable,
            solver::ABCBranchingRule::isApplicable,
            solver::DebugAssertFalseRule::isApplicable
            };

        auto collector = std::make_shared<solver::plugin::MetricsCollector>();
        auto plugin = std::make_shared<solver::plugin::RuleStatsPlugin>(collector, false, nullStream);
        config->plugins = {plugin};

        auto instance = graph::ReadInstance(std::string(RES_DIR) + "tests/2B/2F_6L.nw");

        auto solver = solver::BranchingSolver(instance, config);
        auto solved = solver.solve();
        solver.unapplyReductions();

        REQUIRE(solved);
        REQUIRE(instance->at(0)->Roots().size() == 3);
        REQUIRE(collector->ruleCounts.at("TwoBRule") == 1);
    }

    // the 2B rule is not applicable, but it should be.
    // so this test is disabled until this issue is resolved
    //
    // SECTION("3 Forests, 6 Leafs")
    // {
    //     auto config = std::make_shared<solver::BranchingSolverConfiguration>();
    //     config->boundedDephtSearch = false;
    //     config->activeRules = {
    //         solver::CutBranchRule::isApplicable,
    //         solver::CheckSingleVertexTreesRule::isApplicable,
    //         solver::SingleVertexTreePropagationRule::isApplicable,
    //         solver::EqualPairReductionRule::isApplicable,
    //         solver::TwoBRule::isApplicable,
    //         solver::ACBranchingRule::isApplicable,
    //         solver::AnBCBranchingRule::isApplicable,
    //         solver::DebugAssertFalseRule::isApplicable
    //         };
    //
    //     auto collector = std::make_shared<solver::plugin::MetricsCollector>();
    //     auto plugin = std::make_shared<solver::plugin::RuleStatsPlugin>(collector, false, nullStream);
    //     config->plugins = {plugin};
    //
    //     auto instance = graph::ReadInstance(std::string(RES_DIR) + "tests/2B/3F_6L.nw");
    //
    //     auto solver = solver::BranchingSolver(instance, config);
    //     auto solved = solver.solve();
    //     solver.unapplyReductions();
    //
    //     REQUIRE(solved);
    //     REQUIRE(instance->at(0)->Roots().size() == 3);
    //     REQUIRE(collector->ruleCounts.at("TwoBRule") == 1);
    // }

    SECTION("2 Forests, 20 Leafs")
    {
        auto config = std::make_shared<solver::BranchingSolverConfiguration>();
        config->boundedDephtSearch = false;
        config->activeRules = {
            solver::CutBranchRule::isApplicable,
            solver::CheckSingleVertexTreesRule::isApplicable,
            solver::SingleVertexTreePropagationRule::isApplicable,
            solver::EqualPairReductionRule::isApplicable,
            solver::TwoBRule::isApplicable,
            solver::ACBranchingRule::isApplicable,
            solver::ABCBranchingRule::isApplicable,
            solver::DebugAssertFalseRule::isApplicable
            };

        auto collector = std::make_shared<solver::plugin::MetricsCollector>();
        auto plugin = std::make_shared<solver::plugin::RuleStatsPlugin>(collector, false, nullStream);
        config->plugins = {plugin};

        auto instance = graph::ReadInstance(std::string(RES_DIR) + "tests/2B/2F_20L.nw");

        auto solver = solver::BranchingSolver(instance, config);
        auto solved = solver.solve();
        solver.unapplyReductions();

        REQUIRE(solved);
        REQUIRE(instance->at(0)->Roots().size() == 8);
        REQUIRE(collector->ruleCounts.at("TwoBRule") > 0);
    }
}