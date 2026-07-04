#include <catch2/catch_test_macros.hpp>

#include "../src/Graph/Forest.hpp"
#include "../src/Graph/ForestIO.hpp"
#include "../src/Graph/Instance.hpp"
#include "../src/Solver/BranchingSolver.hpp"
#include "../src/Solver/Rule/ChainReductionRule.hpp"
#include "../src/Solver/Plugin/RuleStatsPlugin.hpp"

#include <iostream>

using namespace graph;
using namespace std;
using namespace solver;

class NullBuffer : public std::streambuf {
public:
    int overflow(int c) override {
        return traits_type::not_eof(c);
    }
};

TEST_CASE("ChainReductionTest - Example instance")
{
    std::stringstream newick;
    newick << "#p 2 11" << endl;
    newick << "((((((((1,2),(3,4)),5),6),10),11),(7,8)),9);" << endl;
    newick << "(((7,9),8),(((((((2,3),1),5),6),10),11),4));" << endl;

    auto instance = graph::ReadInstance(newick);

    auto config = std::make_shared<solver::BranchingSolverConfiguration>();
    config->activeRules = {solver::CutBranchRule::isApplicable,
                           solver::CheckSingleVertexTreesRule::isApplicable,
                           solver::SingleVertexTreePropagationRule::isApplicable,
                           solver::ChainReductionRule::isApplicable,
                           solver::SiblingRuleFactory::allRules,
                           solver::DebugAssertFalseRule::isApplicable};

    NullBuffer nullBuffer;
    std::ostream nullStream(&nullBuffer);
    auto collector = std::make_shared<solver::plugin::MetricsCollector>();
    auto plugin = std::make_shared<solver::plugin::RuleStatsPlugin>(collector, false, nullStream);
    config->plugins = {plugin};

    auto solver = solver::BranchingSolver(instance, config);
    auto solved = solver.solve();
    solver.unapplyReductions();

    REQUIRE(solved);
    REQUIRE(instance->at(0)->isValid());
    CHECK(instance->at(0)->Roots().size() == 4);
    for (int i = 1; i < 12; i++)
    {
        CHECK(instance->at(0)->LabelToTerminal().contains(i));
    }
    CHECK(collector->ruleCounts.at("ChainReductionRule") >= 1);
}