#include <catch2/catch_test_macros.hpp>

#include "../src/Graph/Forest.hpp"
#include "../src/Solver/Action/DeleteEdgeAction.hpp"

using namespace graph;
using namespace std;
using namespace solver;

TEST_CASE("Delete Edge Action - Simple Tree with two Terminals", "[Forest, DeleteEdgeAction, AbstractAction]")
{
    SECTION("Remove edge left child")
    {
        auto f1 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_2_simple.tree",2,1);
        auto f2 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "forest_2_2_singleVertexTrees.tree",2,2);
        auto leftChild = f1.Roots()[0]->leftChild;
        auto rightChild = f1.Roots()[0]->rightChild;

        auto action = DeleteEdgeAction(leftChild, std::make_shared<Forest>(f1));

        INFO("Do Action");
        action.doAction();

        REQUIRE(f1.isValid());
        REQUIRE(f1.Roots().size() == 2);
        REQUIRE(f1.Roots()[0] == leftChild);
        REQUIRE(f1.Roots()[1] == rightChild);
        REQUIRE(f1 == f2);

        INFO("Undo Action");
        action.undoAction();

        auto f1Copy = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_2_simple.tree",2,1);
        REQUIRE(f1.isValid());
        REQUIRE(f1 == f1Copy);

    }

    SECTION("Remove edge from right child")
    {
        auto f1 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_2_simple.tree",2,1);
        auto f2 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "forest_2_2_singleVertexTrees.tree",2,2);
        auto leftChild = f1.Roots()[0]->leftChild;
        auto rightChild = f1.Roots()[0]->rightChild;

        auto action = DeleteEdgeAction(rightChild, std::make_shared<Forest>(f1));

        INFO("Do Action");
        action.doAction();

        REQUIRE(f1.isValid());
        REQUIRE(f1.Roots().size() == 2);
        REQUIRE(f1.Roots()[0] == leftChild);
        REQUIRE(f1.Roots()[1] == rightChild);
        REQUIRE(f1 == f2);

        INFO("Undo Action");
        action.undoAction();

        auto f1Copy = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_2_simple.tree",2,1);
        REQUIRE(f1.isValid());
        REQUIRE(f1 == f1Copy);
    }
}

TEST_CASE("Delete Edge Action - Tree with six Terminals", "[Forest, DeleteEdgeAction, AbstractAction]")
{
    SECTION("Remove inner edge 1")
    {
        auto f1 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example1.tree",6,1);
        auto root = f1.Roots()[0];
        auto node1 = root -> leftChild -> rightChild;
        auto action = DeleteEdgeAction(node1, std::make_shared<Forest>(f1));

        INFO("Do Action");
        action.doAction();

        REQUIRE(f1.isValid());
        REQUIRE(f1.Roots().size() == 2);
        REQUIRE(f1.Roots()[0] == root);
        REQUIRE(f1.Roots()[1] == node1);

        INFO("Undo Action");
        action.undoAction();

        auto f1Copy = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example1.tree",6,1);
        REQUIRE(f1.isValid());
        REQUIRE(f1 == f1Copy);
    }

    SECTION("Remove inner edge 2")
    {
        auto f1 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example1.tree",6,1);
        auto root = f1.Roots()[0];
        auto node2 = root -> leftChild -> leftChild;
        auto action = DeleteEdgeAction(node2, std::make_shared<Forest>(f1));

        INFO("Do Action");
        action.doAction();

        REQUIRE(f1.isValid());
        REQUIRE(f1.Roots().size() == 2);
        REQUIRE(f1.Roots()[0] == node2);
        REQUIRE(f1.Roots()[1] == root);

        INFO("Undo Action");
        action.undoAction();

        auto f1Copy = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example1.tree",6,1);
        REQUIRE(f1.isValid());
        REQUIRE(f1 == f1Copy);
    }

    SECTION("Remove several edges")
    {
        auto f1 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example1.tree",6,1);
        auto root = f1.Roots()[0];
        auto node2 = root->leftChild->leftChild;
        auto node7 = root->leftChild->rightChild->rightChild;
        auto node10 = root->rightChild->rightChild;
        auto action1 = DeleteEdgeAction(node2, std::make_shared<Forest>(f1));
        auto action2 = DeleteEdgeAction(node7, std::make_shared<Forest>(f1));
        auto action3 = DeleteEdgeAction(node10, std::make_shared<Forest>(f1));

        INFO("Do Actions");

        action1.doAction();
        action2.doAction();
        action3.doAction();

        REQUIRE(f1.isValid());
        REQUIRE(f1.Roots().size() == 4);
        REQUIRE(f1.Roots()[0] == node2);
        REQUIRE(f1.Roots()[1] == root);
        REQUIRE(f1.Roots()[2] == node7);
        REQUIRE(f1.Roots()[3] == node10);

        auto f2 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "forest_4_6_simple.tree",6,4);
        REQUIRE(f1 == f2);

        INFO("Undo Actions");
        action3.undoAction();
        action2.undoAction();
        action1.undoAction();

        auto f1Copy = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example1.tree",6,1);
        REQUIRE(f1.isValid());
        REQUIRE(f1 == f1Copy);
    }
}

TEST_CASE("Delete Edge Action - sibling & root order", "[Forest, DeleteEdgeAction, AbstractAction]")
{
    SECTION("Case 1")
    {
        INFO("Remove Edge to left root child");

        auto f1 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "forest_3_12_example3.tree",12,3);
        auto root0 = f1.Roots()[0];
        auto root1 = f1.Roots()[1];
        auto root2 = f1.Roots()[2];
        auto node4 = root1->leftChild;
        auto node11 = root1->rightChild;
        auto action = DeleteEdgeAction(node4, std::make_shared<Forest>(f1));

        INFO("Do Action");
        action.doAction();

        REQUIRE(f1.isValid());
        REQUIRE(f1.Roots().size() == 4);
        REQUIRE(f1.Roots()[0] == root0);
        REQUIRE(f1.Roots()[1] == node4);
        REQUIRE(f1.Roots()[2] == root2);
        REQUIRE(f1.Roots()[3] == node11);

        INFO("Undo Action");
        action.undoAction();

        auto f1Copy = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "forest_3_12_example3.tree",12,3);
        REQUIRE(f1.isValid());
        REQUIRE(f1 == f1Copy);
    }

    SECTION("Case 2")
    {
        INFO("Remove Edge to right root child");

        auto f1 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "forest_3_12_example3.tree",12,3);
        auto root0 = f1.Roots()[0];
        auto root1 = f1.Roots()[1];
        auto root2 = f1.Roots()[2];
        auto node4 = root1->leftChild;
        auto node11 = root1->rightChild;
        auto action = DeleteEdgeAction(node11, std::make_shared<Forest>(f1));

        INFO("Do Action");
        action.doAction();

        REQUIRE(f1.isValid());
        REQUIRE(f1.Roots().size() == 4);
        REQUIRE(f1.Roots()[0] == root0);
        REQUIRE(f1.Roots()[1] == node4);
        REQUIRE(f1.Roots()[2] == root2);
        REQUIRE(f1.Roots()[3] == node11);

        INFO("Undo Action");
        action.undoAction();

        auto f1Copy = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "forest_3_12_example3.tree",12,3);
        REQUIRE(f1.isValid());
        REQUIRE(f1 == f1Copy);
    }


    SECTION("Case 3")
    {
        INFO("Remove Edge to inner node with smallest terminal");

        auto f1 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "forest_3_12_example3.tree",12,3);
        auto root0 = f1.Roots()[0];
        auto root1 = f1.Roots()[1];
        auto root2 = f1.Roots()[2];
        auto node4 = root1->leftChild;
        auto node6 = root1->leftChild->leftChild->leftChild;
        auto node7 = root1->leftChild->leftChild->rightChild;
        auto node8 = root1->leftChild->rightChild;
        auto node11 = root1->rightChild;
        auto action = DeleteEdgeAction(node6, std::make_shared<Forest>(f1));

        INFO("Do Action");
        action.doAction();

        REQUIRE(f1.isValid());
        REQUIRE(f1.Roots().size() == 4);
        REQUIRE(f1.Roots()[0] == root0);
        REQUIRE(f1.Roots()[1] == node6);
        REQUIRE(f1.Roots()[2] == root2);
        REQUIRE(f1.Roots()[3] == root1);

        auto r = f1.Nodes()->at(3);
        REQUIRE(r.leftChild == node11);
        REQUIRE(r.rightChild == node4);
        auto p = f1.Nodes()->at(4);
        REQUIRE(p.leftChild == node8);
        REQUIRE(p.rightChild == node7);


        INFO("Undo Action");
        action.undoAction();

        auto f1Copy = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "forest_3_12_example3.tree",12,3);
        REQUIRE(f1.isValid());
        REQUIRE(f1 == f1Copy);
    }

    SECTION("Case 4")
    {
        INFO("Remove Edge to inner node with high terminals");

        auto f1 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "forest_3_12_example3.tree",12,3);
        auto root0 = f1.Roots()[0];
        auto root1 = f1.Roots()[1];
        auto root2 = f1.Roots()[2];
        auto node5 = root1->leftChild->leftChild;;
        auto node8 = root1->leftChild->rightChild;
        auto node11 = root1->rightChild;
        auto action = DeleteEdgeAction(node8, std::make_shared<Forest>(f1));

        INFO("Do Action");
        action.doAction();

        REQUIRE(f1.isValid());
        REQUIRE(f1.Roots().size() == 4);
        REQUIRE(f1.Roots()[0] == root0);
        REQUIRE(f1.Roots()[1] == root1);
        REQUIRE(f1.Roots()[2] == root2);
        REQUIRE(f1.Roots()[3] == node8);

        auto r = f1.Nodes()->at(3);
        REQUIRE(r.leftChild == node5);
        REQUIRE(r.rightChild == node11);

        INFO("Undo Action");
        action.undoAction();

        auto f1Copy = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "forest_3_12_example3.tree",12,3);
        REQUIRE(f1.isValid());
        REQUIRE(f1 == f1Copy);
    }

}

