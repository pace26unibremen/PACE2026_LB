#include <catch2/catch_test_macros.hpp>

#include "../src/Graph/Forest.hpp"
#include "../src/Graph/ForestIO.hpp"
#include "../src/Graph/Instance.hpp"
#include "../src/Solver/Action/ShortenChainAction.hpp"
#include "../src/Solver/BranchingSolver.hpp"

#include <iostream>

using namespace graph;
using namespace std;
using namespace solver;

TEST_CASE("ShortenChainActionTest - Simple")
{
    std::stringstream newick;
    newick << "(((((((1,2),(3,4)),5),6),10),(7,8)),9);";
    auto forest = std::make_shared<graph::Forest>(graph::ForestIO::ReadNewick(newick,10,1));

    auto shortChainAction = ShortenChainAction(5,10,forest);
    shortChainAction.doAction();
    REQUIRE(forest->isValid());
    CHECK(forest->LabelToTerminal().contains(5));
    CHECK(!forest->LabelToTerminal().contains(6));
    CHECK(!forest->LabelToTerminal().contains(10));

    CHECK(forest->LabelToTerminal().at(5)->parent == forest->LabelToTerminal().at(7)->parent->sibling);
    shortChainAction.undoAction();
    REQUIRE(forest->isValid());
    CHECK(forest->LabelToTerminal().contains(5));
    CHECK(forest->LabelToTerminal().contains(6));
    CHECK(forest->LabelToTerminal().contains(10));
}

TEST_CASE("ShortenChainActionTest - Modified before undo")
{
    SECTION("Cut chain parent")
    {
        std::stringstream newick;
        newick << "(((((((1,2),(3,4)),5),6),10),(7,8)),9);";
        auto forest = std::make_shared<graph::Forest>(graph::ForestIO::ReadNewick(newick,10,1));

        auto shortChainAction = ShortenChainAction(5,10,forest);
        shortChainAction.doAction();
        REQUIRE(forest->isValid());
        CHECK(forest->LabelToTerminal().contains(5));
        CHECK(!forest->LabelToTerminal().contains(6));
        CHECK(!forest->LabelToTerminal().contains(10));

        CHECK(forest->LabelToTerminal().at(5)->parent == forest->LabelToTerminal().at(7)->parent->sibling);

        DeleteEdgeAction edgeDelete(forest->LabelToTerminal().at(5)->parent, forest);
        edgeDelete.doAction();


        shortChainAction.undoAction();
        REQUIRE(forest->isValid());
        CHECK(forest->LabelToTerminal().contains(5));
        CHECK(forest->LabelToTerminal().contains(6));
        CHECK(forest->LabelToTerminal().contains(10));
    }

    SECTION("Cut chain sibling")
    {
        std::stringstream newick;
        newick << "(((((((1,2),(3,4)),5),6),10),(7,8)),9);";
        auto forest = std::make_shared<graph::Forest>(graph::ForestIO::ReadNewick(newick,10,1));

        auto shortChainAction = ShortenChainAction(5,10,forest);
        shortChainAction.doAction();
        REQUIRE(forest->isValid());
        CHECK(forest->LabelToTerminal().contains(5));
        CHECK(!forest->LabelToTerminal().contains(6));
        CHECK(!forest->LabelToTerminal().contains(10));

        CHECK(forest->LabelToTerminal().at(5)->parent == forest->LabelToTerminal().at(7)->parent->sibling);

        DeleteEdgeAction edgeDelete(forest->LabelToTerminal().at(7)->parent, forest);
        edgeDelete.doAction();

        shortChainAction.undoAction();
        REQUIRE(forest->isValid());
        CHECK(forest->LabelToTerminal().contains(5));
        CHECK(forest->LabelToTerminal().contains(6));
        CHECK(forest->LabelToTerminal().contains(10));
    }

    SECTION("Cut chain start")
    {
        std::stringstream newick;
        newick << "(((((((1,2),(3,4)),5),6),10),(7,8)),9);";
        auto forest = std::make_shared<graph::Forest>(graph::ForestIO::ReadNewick(newick,10,1));

        auto shortChainAction = ShortenChainAction(5,10,forest);
        shortChainAction.doAction();
        REQUIRE(forest->isValid());
        CHECK(forest->LabelToTerminal().contains(5));
        CHECK(!forest->LabelToTerminal().contains(6));
        CHECK(!forest->LabelToTerminal().contains(10));

        CHECK(forest->LabelToTerminal().at(5)->parent == forest->LabelToTerminal().at(7)->parent->sibling);

        DeleteEdgeAction edgeDelete(forest->LabelToTerminal().at(5), forest);
        edgeDelete.doAction();

        shortChainAction.undoAction();
        REQUIRE(forest->isValid());
        CHECK(forest->LabelToTerminal().contains(5));
        CHECK(forest->LabelToTerminal().contains(6));
        CHECK(forest->LabelToTerminal().contains(10));
    }
}

