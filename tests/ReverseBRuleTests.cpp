#include <catch2/catch_test_macros.hpp>

#include "../src/Graph/Forest.hpp"
#include "../src/Solver/Rule/ReverseBRule.hpp"

using namespace graph;
using namespace std;
using namespace solver;

TEST_CASE("ReverseBRule - Two Trees", "[Forest, ReverseBRule, AbstractRule]")
{
    SECTION("Cut right sibling c")
    {
        auto f1 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example2.tree",6,1);
        auto f2 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example3.tree",6,1);

        auto instance = std::make_shared<std::vector<std::shared_ptr<Forest>>>();
        instance->push_back(std::make_shared<Forest>(f1));
        instance->push_back(std::make_shared<Forest>(f2));

        auto rule = ReverseBRule::isApplicable(instance, nullptr);

        REQUIRE(rule != nullptr);

        INFO("Apply Rule");
        rule->apply();

        REQUIRE(f1.Roots().size() == 2);
        REQUIRE(f2.Roots().size() == 2);

        // the cut leaf is a root of the new forests, i.e. a single-vertex tree
        auto begin1 = f1.Roots().begin();
        auto end1 = f1.Roots().end();
        Node* cut1 = f1.LabelToTerminal()[2];
        REQUIRE(std::find(begin1, end1, cut1) != end1);

        auto begin2 = f2.Roots().begin();
        auto end2 = f2.Roots().end();
        Node* cut2 = f2.LabelToTerminal()[2];
        REQUIRE(std::find(begin2, end2, cut2) != end2);

        INFO("Unapply Rule");
        rule->unapply();

        auto f1Copy = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example2.tree",6,1);
        auto f2Copy = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example3.tree",6,1);
        REQUIRE(f1 == f1Copy);
        REQUIRE(f2 == f2Copy);
    }

    SECTION("Cut left sibling a")
    {
        auto f1 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example2.tree",6,1);
        auto f2 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example4.tree",6,1);

        auto instance = std::make_shared<std::vector<std::shared_ptr<Forest>>>();
        instance->push_back(std::make_shared<Forest>(f1));
        instance->push_back(std::make_shared<Forest>(f2));

        auto rule = ReverseBRule::isApplicable(instance, nullptr);

        REQUIRE(rule != nullptr);

        INFO("Apply Rule");
        rule->apply();

        REQUIRE(f1.Roots().size() == 2);
        REQUIRE(f2.Roots().size() == 2);

        // the cut leaf is a root of the new forests, i.e. a single-vertex tree
        auto begin1 = f1.Roots().begin();
        auto end1 = f1.Roots().end();
        Node* cut1 = f1.LabelToTerminal()[1];
        REQUIRE(std::find(begin1, end1, cut1) != end1);

        auto begin2 = f2.Roots().begin();
        auto end2 = f2.Roots().end();
        Node* cut2 = f2.LabelToTerminal()[1];
        REQUIRE(std::find(begin2, end2, cut2) != end2);

        INFO("Unapply Rule");
        rule->unapply();

        auto f1Copy = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example2.tree",6,1);
        auto f2Copy = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example4.tree",6,1);
        REQUIRE(f1 == f1Copy);
        REQUIRE(f2 == f2Copy);
    }

    SECTION("Rule not applicable")
    {
        auto f1 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example3.tree",6,1);
        auto f2 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "forest_4_6_simple.tree",6,4);

        auto instance = std::make_shared<std::vector<std::shared_ptr<Forest>>>();
        instance->push_back(std::make_shared<Forest>(f1));
        instance->push_back(std::make_shared<Forest>(f2));

        auto rule = ReverseBRule::isApplicable(instance, nullptr);

        REQUIRE(rule == nullptr);
    }
}