#ifndef PACE2026_TWIN_RELATION_HPP
#define PACE2026_TWIN_RELATION_HPP

#include "../../Graph/Instance.hpp"
#include "LeastCommonAncestor.hpp"

namespace cluster
{

/// \brief This class generates a look up table that links each node to their twins. It is necessary for quick cluster
/// point generation.
class TwinRelation
{

  private:
    std::vector<std::shared_ptr<cluster::LeastCommonAncestor>> LCAs;

    std::unordered_map<graph::Node*, graph::Node*> nodeToTwinBuffer = std::unordered_map<graph::Node*, graph::Node*>();

    void generateInteriorTwinRelation(graph::Node* givenNode,
                                      const std::shared_ptr<cluster::LeastCommonAncestor>& foreignLCA);

    void fuseTwinBufferToSets();

    void prepareLeafTwins(const std::shared_ptr<graph::Forest>& homeForest,
                          const std::shared_ptr<graph::Forest>& foreignForest);

  public:
    /// \brief Returns a pointer to a map which maps each node to their twins.
    explicit TwinRelation(const std::shared_ptr<graph::Instance>& instance);

    /// \brief Map that maps each node of the entire instance to their corresponding twins. You should be enormously
    /// careful when using this map, because not all entries are true equivalence classes.
    /// \note A true equivalence class (the intersection of a node with all the twins and the twins of the twins being
    /// the twins of the node) is also a cluster point. Not all nodes are in a true equivalence class.
    /// Also, the term "true equivalence class" is made up.
    // I am fully aware of the performance implications of a map mapping nodes to sets and the performance implications
    // of the set in general. Shall this cause issues confirmed through profiling we'll implement an arena that handles
    // bulk allocation. Additionally, an instance of this table should theoretically cost around 1 to 2 MB of RAM?
    // (extreme overestimation)
    std::unordered_map<graph::Node*, std::set<graph::Node*>> nodeToTwins =
        std::unordered_map<graph::Node*, std::set<graph::Node*>>();
};

}  //namespace cluster

#endif  //PACE2026_TWIN_RELATION_HPP
