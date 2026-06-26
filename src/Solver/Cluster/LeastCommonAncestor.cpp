#include "LeastCommonAncestor.hpp"

namespace cluster
{
// This design apparently is standard procedure (LCA through Euler Tour, RMQ & Sparse Table)
LeastCommonAncestor::LeastCommonAncestor(const std::shared_ptr<graph::Forest>& forestPointer)
{
    graph::Node* rootNode = forestPointer->Roots().front();

    generatePreorderNumbers(rootNode, 0);

    eulerTour(rootNode, 0);
    precomputeRangeMinimumQuery();
}

int LeastCommonAncestor::generatePreorderNumbers(graph::Node* node, const int preorderNumber)
{
    nodesToPreorderNumber[node] = preorderNumber;

    int current = preorderNumber + 1;
    if (node->leftChild)
        current = generatePreorderNumbers(node->leftChild, current);

    if (node->rightChild)
        current = generatePreorderNumbers(node->rightChild, current);

    return current;
}

void LeastCommonAncestor::eulerTour(graph::Node* node, const int depth)
{

    const int preorderNumber = preorderToNode.size();
    const int eulerNumber = preorderNumbers.size();

    preorderToNode.push_back(node);

    if (preorderToInternalPreorder.size() <= nodesToPreorderNumber[node])
        preorderToInternalPreorder.resize(nodesToPreorderNumber[node] + 1, -1);

    preorderToInternalPreorder[nodesToPreorderNumber.at(node)] = preorderNumber;

    firstOccurences.push_back(eulerNumber);
    levelOfEulerTours.push_back(depth);
    preorderNumbers.push_back(preorderNumber);

    if (const auto leftChild = node->leftChild)
    {
        eulerTour(leftChild, depth + 1);
        levelOfEulerTours.push_back(depth);
        preorderNumbers.push_back(preorderNumber);
    }

    if (const auto rightChild = node->rightChild)
    {
        eulerTour(rightChild, depth + 1);
        levelOfEulerTours.push_back(depth);
        preorderNumbers.push_back(preorderNumber);
    }
}

void LeastCommonAncestor::precomputeRangeMinimumQuery()
{
    rangeMinimumQuery.push_back(preorderNumbers);

    for (unsigned long length = 1; length < preorderNumbers.size(); length *= 2)
    {
        std::vector<int> buffer = std::vector<int>();

        for (unsigned long start = 0; start < preorderNumbers.size() - length; ++start)
        {
            buffer.push_back(computeRangeMinimumQuery(start, start + length));
        }
        rangeMinimumQuery.push_back(buffer);
    }
}

int LeastCommonAncestor::computeRangeMinimumQuery(const unsigned long i, const unsigned long j) const
{
    // This scurrilous construction is a remnant of the reimplementation of rSPR.

    if (i == j)
        return preorderNumbers[i];

    int length = j - i - 1;

    if (length <= 0)
    {
        length = -1;
    }
    else
    {
        length = 31 - __builtin_clz(length);
    }

    const int rmq1 = rangeMinimumQuery[length + 1][i];

    int rmq2;

    if (length >= 0)
    {
        rmq2 = rangeMinimumQuery[length + 1][j - (1 << (length))];
    }
    else
    {
        rmq2 = rangeMinimumQuery[length + 1][j];
    }

    if (rmq1 < rmq2)
        return rmq1;

    return rmq2;
}

graph::Node* LeastCommonAncestor::getLeastCommonAncestor(graph::Node* firstNode, graph::Node* secondNode)
{

    if (not nodesToPreorderNumber.contains(firstNode) or not nodesToPreorderNumber.contains(secondNode))
        throw std::invalid_argument("One of the nodes is not contained within this structure. "
                                    "Did you accidentally pass a node from another tree?");

    const int preorderA = preorderToInternalPreorder[nodesToPreorderNumber[firstNode]];
    const int preorderB = preorderToInternalPreorder[nodesToPreorderNumber[secondNode]];

    int leastCommonAncestorIndex;

    if (preorderA <= preorderB)  // This if / else actually defines LCA(A,B) = LCA(B,A)
    {
        leastCommonAncestorIndex = computeRangeMinimumQuery(firstOccurences[preorderA], firstOccurences[preorderB]);
    }
    else
    {
        leastCommonAncestorIndex = computeRangeMinimumQuery(firstOccurences[preorderB], firstOccurences[preorderA]);
    }

    return preorderToNode[leastCommonAncestorIndex];
}

}  //namespace cluster
