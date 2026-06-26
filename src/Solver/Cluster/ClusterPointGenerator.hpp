#ifndef PACE2026_CLUSTER_POINT_GENERATOR_HPP
#define PACE2026_CLUSTER_POINT_GENERATOR_HPP

#include "TwinRelation.hpp"
namespace cluster
{

/// \brief This class recursively sieves through each point and checks whether one point is a cluster point
/// through checking label equivalence and twin equivalence. Refer to the dissertation of C. Whidden.
class ClusterPointGenerator
{
  private:
    void generateClusterPoints(graph::Node* node);

    bool trueEquivalenceClass(graph::Node* node) const;

    bool leafEquivalent(graph::Node* node) const;

  public:
    /// \brief This is the constructor of the ClusterPointGenerator.
    /// \param instance The instance for which we want to generate cluster points.
    /// \param twinRelation The table that links each note to their corresponding twins.
    /// \note A node is a cluster point if, and only if, they're: not the root, not a leaf, have both children
    /// & are in a true equivalence class.
    ClusterPointGenerator(const std::shared_ptr<graph::Instance>& instance, const cluster::TwinRelation& twinRelation);

    /// \brief This static function returns an instance of the ClusterPointGenerator without the hassle of having to
    /// generate the Twin-Table object first. The table will be generated internally and discarded after usage, which
    /// has to be kept in mind when using this function. (Potential time and memory cost!)
    /// \note Access to Cluster Point vector through public member access.
    static ClusterPointGenerator wrappedConstructor(const std::shared_ptr<graph::Instance>& instance);

    /// \brief This is the vector that contains the cluster points of a given instance. All of the points within
    /// this vector are from the front-tree of the instance and in order to use them one must fetch the twins of a node
    /// within this vector and fetch the twins of the clusterpoint through the Twin-Map.
    std::vector<graph::Node*> clusterPoints = std::vector<graph::Node*>();

    cluster::TwinRelation twinRelation;
};

}  //namespace cluster

#endif  //PACE2026_CLUSTER_POINT_GENERATOR_HPP
