#ifndef PACE2026_LEAST_COMMON_ANCESTOR_HPP
#define PACE2026_LEAST_COMMON_ANCESTOR_HPP

#include "../../Graph/Forest.hpp"

namespace cluster
{

/// \brief This class generates the table of least common ancestors for a given Forest.
/// It is necessary to generate cluster points.
class LeastCommonAncestor
{

  private:
    std::unordered_map<graph::Node*, int> nodesToPreorderNumber = std::unordered_map<graph::Node*, int>();

    std::vector<int> preorderNumbers = std::vector<int>();

    std::vector<int> levelOfEulerTours = std::vector<int>();
    std::vector<int> firstOccurences = std::vector<int>();

    std::vector<int> preorderToInternalPreorder = std::vector<int>();
    std::vector<graph::Node*> preorderToNode = std::vector<graph::Node*>();
    std::vector<std::vector<int>> rangeMinimumQuery = std::vector<std::vector<int>>();

    int generatePreorderNumbers(graph::Node* node, int preorderNumber);

    void eulerTour(graph::Node* node, int depth);

    void precomputeRangeMinimumQuery();
    int computeRangeMinimumQuery(unsigned long i, unsigned long j) const;

  public:
    /// \brief This is the constructor of the LCA Table. It provides the fetching of a LCA Node in constant time.
    /// \note This data structure currently has to be reinstantiated if the structure of the tree changes!
    /// \param forestPointer The forest of which we want to generate the LCA table for.
    explicit LeastCommonAncestor(const std::shared_ptr<graph::Forest>& forestPointer);

    /// \brief This function returns a node pointer to a least common ancestor of two given nodes.
    /// The two given nodes MUST be from the forest which created the LCA object.
    /// \note This operates in constant time O(1).
    /// \return Least Common Ancestor
    graph::Node* getLeastCommonAncestor(graph::Node* firstNode, graph::Node* secondNode);
};

}  //namespace cluster

#endif  //PACE2026_LEAST_COMMON_ANCESTOR_HPP

