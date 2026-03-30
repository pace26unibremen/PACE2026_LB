#include <catch2/catch_test_macros.hpp>

#include "../../src/Graph/Instance.hpp"
#include "../../src/Solver/ILP/TreeUtils.hpp"
#include "../../src/Cluster/LeastCommonAncestor.hpp"
#include <iostream>

using namespace graph;
using namespace std;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Loads a two-tree instance and returns both forests.
static std::shared_ptr<graph::Forest>
loadForest(const std::string& filename, int nLeaves, int nTrees)
{
    auto forest = std::make_shared<graph::Forest>(std::string(ILP_TEST_DIR) + filename, nLeaves, nTrees);

    return forest;
}

// Builds an empty forest with no nodes and no roots.
static std::shared_ptr<graph::Forest> buildEmptyForest()
{
    return std::make_shared<graph::Forest>(
        std::make_shared<std::vector<graph::Node>>(),
        std::make_shared<std::unordered_map<graph::Node*, unsigned int>>(),
        std::make_shared<std::unordered_map<unsigned int, graph::Node*>>(),
        std::make_shared<std::vector<graph::Node*>>()
    );
}

// ---------------------------------------------------------------------------
// getRootIndex
// ---------------------------------------------------------------------------

TEST_CASE("getRootIndex", "[TreeUtils]")
{
    SECTION("minimal_4leaves: root index is valid")
    {
        auto f1 = loadForest("ilp_minimal_4leaves.tree", 4, 1);

        REQUIRE(f1->Roots().size() == 1);

        INFO("Get Root Index of forest1");
        int root1 = solver::getRootIndex(*f1);

        REQUIRE(root1 >= 0);
        REQUIRE(root1 < static_cast<int>(f1->Nodes().size()));
        REQUIRE(f1->Nodes()[root1].parent == nullptr);
    } 

    SECTION("empty forest: throws logic_error")
    {
        auto emptyForest = buildEmptyForest();
        REQUIRE_THROWS_AS(solver::getRootIndex(*emptyForest), std::logic_error);
    }
}

// ---------------------------------------------------------------------------
// buildNodeToIndexMap
// ---------------------------------------------------------------------------

TEST_CASE("buildNodeToIndexMap", "[TreeUtils]")
{
    SECTION("minimal_4leaves: map covers all nodes")
    {
        auto f1 = loadForest("ilp_minimal_4leaves.tree", 4, 1);

        auto map1 = solver::buildNodeToIndexMap(*f1);
        int n = static_cast<int>(f1->Nodes().size());

        // Map has same size as Nodes()
        REQUIRE(map1.size() == f1->Nodes().size());

        // All Nodes of Forest are present in map...
        for (const auto& node : f1->Nodes())
        {
            REQUIRE(map1.count(&node) == 1);
        }

        // Valid Range & Unique indizes
        std::set<int> seenIndices;
        for (const auto& [ptr, idx] : map1)
        {
            REQUIRE(idx >= 0);
            REQUIRE(idx < n);
            REQUIRE(seenIndices.count(idx) == 0);
            seenIndices.insert(idx);
        }
    }

    SECTION("empty forest: map is empty")
    {
        auto emptyForest = buildEmptyForest();
        auto map = solver::buildNodeToIndexMap(*emptyForest);
        REQUIRE(map.empty());
    }
}

// ---------------------------------------------------------------------------
// buildIndexToNodeMap
// ---------------------------------------------------------------------------

TEST_CASE("buildIndexToNodeMap", "[TreeUtils]")
{
    SECTION("minimal_4leaves: map covers all nodes")
    {
        auto f1 = loadForest("ilp_minimal_4leaves.tree", 4, 1);
        auto map1 = solver::buildIndexToNodeMap(*f1);
        int n = static_cast<int>(f1->Nodes().size());

        INFO("map size must equal number of nodes");
        REQUIRE(map1.size() == f1->Nodes().size());

        INFO("all indices must be in valid range [0, nNodes)");
        for (const auto& [idx, nodePtr] : map1)
        {
            REQUIRE(idx >= 0);
            REQUIRE(idx < n);
        }

        INFO("all node pointers must point to a node in Nodes()");
        for (const auto& [idx, nodePtr] : map1)
        {
            bool found = false;
            for (const auto& node : f1->Nodes())
            {
                if (&node == nodePtr) { found = true; break; }
            }
            REQUIRE(found);
        }

        INFO("all indices must be unique");
        std::set<int> seenIndices;
        for (const auto& [idx, nodePtr] : map1)
        {
            REQUIRE(seenIndices.count(idx) == 0);
            seenIndices.insert(idx);
        }
    }

    SECTION("empty forest: map is empty")
    {
        auto emptyForest = buildEmptyForest();
        auto map = solver::buildIndexToNodeMap(*emptyForest);
        REQUIRE(map.empty());
    }
}

// ---------------------------------------------------------------------------
// getPath
// ---------------------------------------------------------------------------

TEST_CASE("getPath", "[TreeUtils]")
{
    SECTION("minimal_4leaves")
    {
        auto f1 = loadForest("ilp_minimal_4leaves.tree", 4, 1);
        cluster::LeastCommonAncestor lca1(f1);
        auto& labelToTerminal = f1->LabelToTerminal();
        auto map1 = solver::buildNodeToIndexMap(*f1);
        int nEdges = static_cast<int>(f1->Nodes().size());
        int rootIdx = solver::getRootIndex(*f1);

        INFO("path to itself should be empty");
        for (const auto& [label, nodePtr] : f1->LabelToTerminal())
        {
            auto path = solver::getPath(*f1, label, label, lca1, map1);
            REQUIRE(path.empty());
        }

        INFO("path between siblings contains exactly 2 edges (nodes of siblings)");
        auto path12 = solver::getPath(*f1, 1, 2, lca1, map1);

        INFO("path between siblings must contain exactly 2 edges");
        REQUIRE(path12.size() == 2);

        // Internal index is different than label of leaf...
        int idx1 = map1.at(labelToTerminal.at(1));
        int idx2 = map1.at(labelToTerminal.at(2));

        REQUIRE(path12.count(idx1) > 0.5 );
        REQUIRE(path12.count(idx2) > 0.5 );

        INFO("path between siblings must be symmetric");
        auto path21 = solver::getPath(*f1, 2, 1, lca1, map1);
        REQUIRE(path12 == path21);

        INFO("all edge indices must lie in valid range [0, nEdges) & must be keys in nodeToIndexMap");
        for (int idx : path12)
        {
            REQUIRE(idx >= 0);
            REQUIRE(idx < nEdges);
            bool found = false;
            for (const auto& [ptr, mapIdx] : map1)
            {
                if (mapIdx == idx) { found = true; break; }
            }
            REQUIRE(found);
        }

        INFO("longest path...");
        auto path13 = solver::getPath(*f1, 1, 3, lca1, map1);
        REQUIRE(path13.size() == 4);

        INFO("root index must never appear in any path");
        for (const auto& [labelA, ptrA] : f1->LabelToTerminal())
        {
            for (const auto& [labelB, ptrB] : f1->LabelToTerminal())
            {
                auto path = solver::getPath(*f1, labelA, labelB, lca1, map1);
                REQUIRE(path.count(rootIdx) == 0);
            }
        }
    }

    SECTION("caterpillar_5leaves")
    {
        auto f1 = loadForest("ilp_caterpillar_5leaves.tree", 5, 1);
        cluster::LeastCommonAncestor lca1(f1);
        auto map1 = solver::buildNodeToIndexMap(*f1);
        int nEdges = static_cast<int>(f1->Nodes().size());
        int rootIdx = solver::getRootIndex(*f1);

        INFO("path to itself should be empty");
        for (const auto& [label, nodePtr] : f1->LabelToTerminal())
        {
            auto path = solver::getPath(*f1, label, label, lca1, map1);
            REQUIRE(path.empty());
        }

        auto path12 = solver::getPath(*f1, 1, 2, lca1, map1);
        auto path13 = solver::getPath(*f1, 1, 3, lca1, map1);
        auto path14 = solver::getPath(*f1, 1, 4, lca1, map1);
        auto path15 = solver::getPath(*f1, 1, 5, lca1, map1);


        INFO("monotonicity: path(1,5) > path(1,2) in caterpillar");
        REQUIRE(path15.size() > path14.size());
        REQUIRE(path14.size() > path13.size());
        REQUIRE(path13.size() > path12.size());
    }
}

// ---------------------------------------------------------------------------
// areTwoPathsDisjoint
// ---------------------------------------------------------------------------

TEST_CASE("areTwoPathsDisjoint", "[TreeUtils]")
{
    auto f1 = loadForest("ilp_minimal_4leaves.tree", 4, 1);
    cluster::LeastCommonAncestor lca1(f1);
    auto map1 = solver::buildNodeToIndexMap(*f1);

    SECTION("ilp_minimal_4leaves: both empty paths should always throw logic_error")
    {
        REQUIRE_THROWS_AS(solver::areTwoPathsDisjoint(*f1, 1, 1, 2, 2, lca1, map1), std::logic_error);
    }

    SECTION("ilp_minimal_4leaves: one empty path should always throw logic_error")
    {
        REQUIRE_THROWS_AS(solver::areTwoPathsDisjoint(*f1, 1, 1, 1, 2, lca1, map1), std::logic_error);
    }
    
    SECTION("ilp_minimal_4leaves: identical paths should always return false")
    {
        bool disjoint_1212 = solver::areTwoPathsDisjoint(*f1, 1, 2, 1, 2, lca1, map1);
        REQUIRE_FALSE(disjoint_1212);
    }
    
    SECTION("ilp_minimal_4leaves: sibling pairs (1,2) and (3,4) are disjoint")
    {
        bool disjoint_1234 = solver::areTwoPathsDisjoint(*f1, 1, 2, 3, 4, lca1, map1);
        REQUIRE(disjoint_1234);
    }
    
    SECTION("ilp_minimal_4leaves: shared leaf in paths should always return false")
    {
        bool disjoint_1223 = solver::areTwoPathsDisjoint(*f1, 1, 2, 2, 3, lca1, map1);
        REQUIRE_FALSE(disjoint_1223);
    }
    
    SECTION("ilp_minimal_4leaves: shared edge shold always return false")
    {
        bool disjoint_1324 = solver::areTwoPathsDisjoint(*f1, 1, 3, 2, 4, lca1, map1);
        REQUIRE_FALSE(disjoint_1324);
    }
    
    SECTION("ilp_minimal_4leaves: symmetrical paths should have the same effect")
    {
        bool disjoint_4231 = solver::areTwoPathsDisjoint(*f1, 4, 2, 3, 1, lca1, map1);
        REQUIRE(!disjoint_4231);
    }
}

// ---------------------------------------------------------------------------
// checkIncompatibleTriple
// ---------------------------------------------------------------------------

TEST_CASE("checkIncompatibleTriple", "[TreeUtils]")
{
    auto f1 = loadForest("ilp_minimal_4leaves.tree", 4, 1);
    auto f2 = loadForest("ilp_asymmetric_4leaves.tree", 4, 1);
    cluster::LeastCommonAncestor lca1(f1);
    cluster::LeastCommonAncestor lca2(f2);
    auto map1 = solver::buildNodeToIndexMap(*f1);
    auto map2 = solver::buildNodeToIndexMap(*f2);

    SECTION("same leafs should always result in a logic_error...")
    {
        REQUIRE_THROWS_AS(solver::checkIncompatibleTriple(1, 1, 1, *f1, *f2, lca1, lca2, map1, map2), std::logic_error);
        REQUIRE_THROWS_AS(solver::checkIncompatibleTriple(1, 2, 1, *f1, *f2, lca1, lca2, map1, map2), std::logic_error);
        REQUIRE_THROWS_AS(solver::checkIncompatibleTriple(2, 1, 1, *f1, *f2, lca1, lca2, map1, map2), std::logic_error);
        REQUIRE_THROWS_AS(solver::checkIncompatibleTriple(1, 1, 2, *f1, *f2, lca1, lca2, map1, map2), std::logic_error);
    }

    SECTION("[1,2,3] should be an incompatible triple")
    {
        REQUIRE(solver::checkIncompatibleTriple(1, 2, 3, *f1, *f2, lca1, lca2, map1, map2));
        REQUIRE(solver::checkIncompatibleTriple(2, 1, 3, *f1, *f2, lca1, lca2, map1, map2));
        REQUIRE(solver::checkIncompatibleTriple(3, 2, 1, *f1, *f2, lca1, lca2, map1, map2));
    }

    SECTION("[1,2,4] should be an compatible triple")
    {
        REQUIRE_FALSE(solver::checkIncompatibleTriple(1, 2, 4, *f1, *f2, lca1, lca2, map1, map2));
        REQUIRE_FALSE(solver::checkIncompatibleTriple(2, 1, 4, *f1, *f2, lca1, lca2, map1, map2));
        REQUIRE_FALSE(solver::checkIncompatibleTriple(4, 2, 1, *f1, *f2, lca1, lca2, map1, map2));
    }

    SECTION("[1,3,4] should be an incompatible triple")
    {
        REQUIRE(solver::checkIncompatibleTriple(1, 4, 3, *f1, *f2, lca1, lca2, map1, map2));
        REQUIRE(solver::checkIncompatibleTriple(4, 1, 3, *f1, *f2, lca1, lca2, map1, map2));
        REQUIRE(solver::checkIncompatibleTriple(3, 4, 1, *f1, *f2, lca1, lca2, map1, map2));
    }

    SECTION("[2,3,4] should be an incompatible triple")
    {
        REQUIRE(solver::checkIncompatibleTriple(2, 4, 3, *f1, *f2, lca1, lca2, map1, map2));
        REQUIRE(solver::checkIncompatibleTriple(4, 2, 3, *f1, *f2, lca1, lca2, map1, map2));
        REQUIRE(solver::checkIncompatibleTriple(3, 4, 2, *f1, *f2, lca1, lca2, map1, map2));
    }
}