#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include "../src/Cluster/ClusterInstance.hpp"
#include "../src/Cluster/TwinRelation.hpp"
#include "../src/Graph/Forest.hpp"
#include "../src/Graph/Instance.hpp"
#include "functional"



// This tests the functionality of the LCA Lookup Table.
// Test Question: Let A and B be nodes: Does
// Forall Forests (of an instance), Forall A, Forall B : LCA(A,B) = LCA(B,A) = naiveLCA(A,B) = naiveLCA(B,A)
// hold true?
TEST_CASE("LeastCommonAncestorTableCorrectness")
{
    SECTION("Check if the fetching of LCA Nodes is symmetric and the same result to a naive implementation.")
    {

        std::function<graph::Node*(graph::Node*, graph::Node*)> naiveLCA =
            [&](graph::Node* nodeA, graph::Node* nodeB) -> graph::Node* {

                std::set<graph::Node*> traversedPoints = std::set<graph::Node*>();

                if (nodeA == nullptr or nodeB == nullptr)
                    return nullptr;

                if (nodeA == nodeB)
                    return nodeA;


                traversedPoints.insert(nodeA);
                traversedPoints.insert(nodeB);

                graph::Node* bufferA = nodeA;
                graph::Node* bufferB = nodeB;

                // This is a naive LCA Algorithm. We traverse upwards for both nodes and return the node that "clashes"
                // i.e.  the first node that is already within the set of collected references.
                while (true)
                {
                    if (bufferA and bufferA->parent and (bufferA = bufferA->parent) != nullptr)
                    {
                        if (not traversedPoints.contains(bufferA))
                            traversedPoints.insert(bufferA);
                        else return bufferA;
                    }

                    if (bufferB and bufferB->parent and (bufferB = bufferB->parent) != nullptr)
                    {
                        if (not traversedPoints.contains(bufferB))
                            traversedPoints.insert(bufferB);
                        else return bufferB;
                    }

                    if (bufferA ==nullptr and bufferB == nullptr)
                        return nullptr;

                }
        };

        // For convenience, we fetch all the pointers to nodes of a given Forest beginning from (a) root node.
        std::function<void(std::set<graph::Node*>*, graph::Node*)> fetchNodePointerSet =
            [&](std::set<graph::Node*>* set, graph::Node* node) -> void {

                if (node)
                    set->insert(node);
                else return;

                if (node->leftChild) fetchNodePointerSet(set, node->leftChild);
                if (node->rightChild) fetchNodePointerSet(set, node->rightChild);
        };


        std::shared_ptr<graph::Instance>  instance = graph::ReadInstance(std::string(TEST_EXAMPLES_DIR) + "forest_100_11_stressTest.tree");

        bool isValid = true;


        for (const std::shared_ptr<graph::Forest>& forest :  *instance)
        {
            std::set<graph::Node*> allNodesOfForest = std::set<graph::Node*>();
            fetchNodePointerSet(&allNodesOfForest, forest->Roots().front());

            cluster::LeastCommonAncestor LCA = cluster::LeastCommonAncestor(forest);


            for (const auto& outerNodeLoop : allNodesOfForest)
            {

                for (const auto& innerNodeLoop : allNodesOfForest)
                {

                    graph::Node* naive = naiveLCA(outerNodeLoop, innerNodeLoop);
                    graph::Node* naivePrime = naiveLCA(innerNodeLoop, outerNodeLoop);

                    if (naive == nullptr)
                    {
                        isValid = false;
                        goto exit;
                    }

                    graph::Node* firstLCA = LCA.getLeastCommonAncestor(outerNodeLoop, innerNodeLoop);
                    graph::Node* secondLCA = LCA.getLeastCommonAncestor(innerNodeLoop, outerNodeLoop);

                    if (firstLCA == nullptr or secondLCA == nullptr)
                    {
                        isValid = false;
                        goto exit;
                    }


                    isValid &= ((firstLCA == secondLCA) and (firstLCA == naive) and (naive == naivePrime));
                    if (not isValid) goto exit;
                }

            }




        }

        exit:


        REQUIRE(isValid);
    }
}