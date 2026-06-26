#include <catch2/catch_test_macros.hpp>

#include "../src/Graph/Forest.hpp"
#include "../src/Graph/ForestIO.hpp"

#include <fstream>
#include <sstream>

TEST_CASE("Read and Write single Tree", "[Forest, ForestIO]")
{
    SECTION("Example 1: Tree with 2 Terminals")
    {
        auto t1 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_2_simple.tree", 2, 1);
        SECTION("Read")
        {
            INFO("compare number of nodes");
            REQUIRE(t1.Nodes()->size() == 3);
            INFO("Check number of trees");
            REQUIRE(t1.Roots().size() == 1);
            INFO("Check number of terminals");
            REQUIRE(t1.TerminalToLabel().size() == 2);
            INFO("check validity of tree structure");
            REQUIRE(t1.isValid());
        }
        SECTION("Write")
        {
            std::ifstream t1StreamOriginal(std::string(TEST_EXAMPLES_DIR) + "tree_1_2_simple.tree");
            std::stringstream t1StreamWrite;
            t1.write(t1StreamWrite);

            std::string t1Original;
            std::string t1Write;

            INFO("compare newick representation before and after write");
            getline(t1StreamOriginal, t1Original);
            getline(t1StreamWrite, t1Write);
            REQUIRE(t1Write == t1Original);

            // the streams should contain a remaining empty line
            getline(t1StreamOriginal, t1Original);
            getline(t1StreamWrite, t1Write);
            REQUIRE(t1Write == t1Original);
        }
    }

    SECTION("Example 2: Tree with 6 Terminals")
    {
        auto t2 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example1.tree", 6, 1);
        SECTION("Read")
        {
            INFO("compare number of nodes");
            REQUIRE(t2.Nodes()->size() == 11);
            INFO("Check number of trees");
            REQUIRE(t2.Roots().size() == 1);
            INFO("Check number of terminals");
            REQUIRE(t2.TerminalToLabel().size() == 6);
            INFO("check validity of tree structure");
            REQUIRE(t2.isValid());
        }
        SECTION("Write")
        {
            std::ifstream t2StreamOriginal(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example1.tree");
            std::stringstream t2StreamWrite;
            t2.write(t2StreamWrite);

            std::string t2Original;
            std::string t2Write;

            INFO("compare newick representation before and after write");
            getline(t2StreamOriginal, t2Original);
            getline(t2StreamWrite, t2Write);
            REQUIRE(t2Write == t2Original);

            // the streams should contain a remaining empty line
            getline(t2StreamOriginal, t2Original);
            getline(t2StreamWrite, t2Write);
            REQUIRE(t2Write == t2Original);
        }
    }

    SECTION("Example 3: Tree with 6 Terminals")
    {
        auto t3 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example2.tree", 6, 1);
        SECTION("Read")
        {
            INFO("compare number of nodes");
            REQUIRE(t3.Nodes()->size() == 11);
            INFO("Check number of trees");
            REQUIRE(t3.Roots().size() == 1);
            INFO("Check number of terminals");
            REQUIRE(t3.TerminalToLabel().size() == 6);
            INFO("check validity of tree structure");
            REQUIRE(t3.isValid());
        }

        SECTION("Write")
        {
            std::ifstream t3StreamOriginal(std::string(TEST_EXAMPLES_DIR) + "tree_1_6_example2.tree");
            std::stringstream t3StreamWrite;
            t3.write(t3StreamWrite);

            std::string t3Original;
            std::string t3Write;

            INFO("compare newick representation before and after write");
            getline(t3StreamOriginal, t3Original);
            getline(t3StreamWrite, t3Write);
            REQUIRE(t3Write == t3Original);

            getline(t3StreamOriginal, t3Original);
            getline(t3StreamWrite, t3Write);
            REQUIRE(t3Write == t3Original);
        }
    }
}

TEST_CASE("Read and Write Forest", "[Forest, ForestIO]")
{
    SECTION("Example 1: Forest with 2 Trees and 2 Terminals")
    {
        auto f1 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "forest_2_2_singleVertexTrees.tree",2,2);
        SECTION("Read")
        {
            INFO("compare number of nodes");
            REQUIRE(f1.Nodes()->size() == 2);
            INFO("Check number of trees");
            REQUIRE(f1.Roots().size() == 2);
            INFO("Check number of terminals");
            REQUIRE(f1.TerminalToLabel().size() == 2);
            INFO("check validity of tree structure");
            REQUIRE(f1.isValid());
        }

        SECTION("Write")
        {
            std::ifstream streamOriginal(std::string(TEST_EXAMPLES_DIR) + "forest_2_2_singleVertexTrees.tree");
            std::stringstream streamWrite;
            f1.write(streamWrite);

            std::string original;
            std::string write;

            INFO("compare newick representation before and after write");
            getline(streamOriginal, original);
            getline(streamWrite, write);
            REQUIRE(write == original);

            getline(streamOriginal, original);
            getline(streamWrite, write);
            REQUIRE(write == original);

            getline(streamOriginal, original);
            getline(streamWrite, write);
            REQUIRE(write == original);
        }
    }

    SECTION("Example 2: Forest with 4 Trees and 6 Terminals")
    {
        auto f2 = graph::Forest(std::string(TEST_EXAMPLES_DIR) + "forest_4_6_simple.tree",6,4);
        SECTION("Read")
        {
            INFO("compare number of nodes");
            REQUIRE(f2.Nodes()->size() == 8);
            INFO("Check number of trees");
            REQUIRE(f2.Roots().size() == 4);
            INFO("Check number of terminals");
            REQUIRE(f2.TerminalToLabel().size() == 6);
            INFO("check validity of tree structure");
            REQUIRE(f2.isValid());
        }

        SECTION("Write")
        {
            std::ifstream streamOriginal(std::string(TEST_EXAMPLES_DIR) + "forest_4_6_simple.tree");
            std::stringstream streamWrite;
            f2.write(streamWrite);

            std::string original;
            std::string write;

            INFO("compare newick representation before and after write");
            getline(streamOriginal, original);
            getline(streamWrite, write);
            REQUIRE(write == original);

            getline(streamOriginal, original);
            getline(streamWrite, write);
            REQUIRE(write == original);

            getline(streamOriginal, original);
            getline(streamWrite, write);
            REQUIRE(write == original);

            getline(streamOriginal, original);
            getline(streamWrite, write);
            REQUIRE(write == original);

            getline(streamOriginal, original);
            getline(streamWrite, write);
            REQUIRE(write == original);
        }
    }
}
