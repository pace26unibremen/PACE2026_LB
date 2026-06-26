#ifndef PACE2026_CLUSTER_SOLVER_HPP
#define PACE2026_CLUSTER_SOLVER_HPP

#include "../AbstractSolver.hpp"
#include "../Action/DecoupleSubtreeAction.hpp"

#include <list>
#include <stack>

namespace solver
{

class ClusterRange;

/// \brief The cluster solver performs the cluster reduction.
///
/// Example code,
/// How to use the cluster solver and solve the single clusters with a branching solver:
/// \code
/// auto instance = ReadInstance(...);
/// auto cs = solver::ClusterSolver(instance);
/// cs.solve();
/// for (const auto& [cluster,context] : cs.Clusters())
/// {
///     auto config = std::make_shared<solver::BranchingSolverConfiguration>();
///     auto bs = solver::BranchingSolver(cluster,config, context);
///     bs.solve();
///     bs.unapplyReductions();
/// }
/// cs.unapplyReductions();
/// \endcode
class ClusterSolver : public AbstractSolver
{
    friend ClusterRange;

    /// \brief the cluster range,
    /// for the iterative solving of the cluster sequence,
    /// it manages the relevant intermediate steps.
    /// \ref ClusterRangeIterator
    /// \ref collectCuttedClusterRoot
    /// \ref cutClusterTerminals
    std::shared_ptr<ClusterRange> clusterRange;

    /// \brief a vector of all clusters
    std::vector<std::shared_ptr<graph::Instance>> cluster;

    /// \brief A collection of all cluster roots that have been cutted.
    /// \note The term cluster root actually refers to the terminal, that annotates the actual root as a sibling.
    std::unordered_set<unsigned int> cuttedClusterRoots;

    /// \brief A list with an entry for each cluster.
    /// Each entry is itself a list that holds for each forest of the instance, the forest and the cluster point.
    ///
    /// The structure is organized as follows:
    /// \code
    /// pointsAndForests_perCluster
    /// ├── Cluster 0
    /// │   ├── (shared_ptr<Forest>, Node*)  — forest and its cluster point
    /// │   ├── (shared_ptr<Forest>, Node*)
    /// │   └── ...
    /// ├── Cluster 1
    /// │   ├── (shared_ptr<Forest>, Node*)
    /// │   └── ...
    /// └── ...
    /// \endcode

    std::list<std::list<std::pair<std::shared_ptr<graph::Forest>, graph::Node*>>> pointsAndForests_perCluster =
        std::list<std::list<std::pair<std::shared_ptr<graph::Forest>, graph::Node*>>>();

    /// \brief A set of all cluster terminals that where introduced.
    /// \note The term cluster terminal refers to those terminals, that replace the cluster subtrees in the parent tree.
    std::unordered_set<unsigned int> clusterTerminal = std::unordered_set<unsigned int>();
    /// \brief A set of all cluster roots that where introduced.
    /// \note The term cluster root actually refers to the terminal, that annotates the actual root as a sibling.
    std::unordered_set<unsigned int> clusterRoot = std::unordered_set<unsigned int>();

    /// \brief A map that maps a cluster to its label of the cluster root.
    /// The parent cluster maps to 0.
    /// \note The term cluster root actually refers to the terminal, that annotates the actual root as a sibling.
    std::unordered_map<std::shared_ptr<graph::Instance>, unsigned int> clusterToRootLabel;
    /// \brief A map that maps the label of the cluster root to the corresponding cluster.
    /// O maps to the parent cluster.
    /// \note The term cluster root actually refers to the terminal, that annotates the actual root as a sibling.
    std::unordered_map<unsigned int, std::shared_ptr<graph::Instance>> rootLabelToCluster;
    /// \brief A map that maps a cluster to its cluster terminals.
    /// \note The term cluster terminal refers to those terminals, that replace the cluster subtrees in the parent tree.
    std::unordered_map<std::shared_ptr<graph::Instance>, std::unordered_set<unsigned int>> clusterToClusterLabels;

    /// \brief The decoupling actions on the first forest.
    std::stack<DecoupleSubtreeAction> changesOnF0 = std::stack<DecoupleSubtreeAction>();
    /// \brief The decoupling actions on all other forests.
    /// We will not need them after the initial doAction, but we have to store them to avoid gc.
    std::stack<DecoupleSubtreeAction> changes = std::stack<DecoupleSubtreeAction>();

    /// \brief Building a new instance (a cluster) from the given instance,
    /// which just contains the i-th tree of each forest.
    /// The new instance shares the \ref Forest::Nodes vectors with the original forests
    /// and has independent roots vectors and independent label / terminal maps.
    /// \param i the index of the root / component, from which the new instance is constructed.
    /// \returns new sub instance
    std::shared_ptr<graph::Instance> buildSingleCluster(unsigned int i);

    /// Gets the cluster points from the cluster point generator and fills \ref pointsAndForests_perCluster
    void getClusterPoints();

    /// \brief We introduce new labels,
    /// so we may need to resize the subtreeTerminals field of \ref graph::Node.
    void resizeSubtreeTerminalsVector();

    /// \brief Performs the decoupling of the cluster subtrees within the instance.
    /// \brief Fills the \ref clusterTerminal and \ref clusterRoot field.
    void decoupleSubtrees();

    /// \brief Split up the instance in clusters.
    /// Fills the \ref cluster, \ref clusterToRootLabel, \ref rootLabelToCluster and \ref clusterToClusterLabels
    /// member fields.
    void splitInstance();

    /// \brief Sorts \ref cluster in post-order (children before parents) via DFS.
    ///
    /// Traverses the cluster tree starting at root label 0. The resulting order
    /// ensures every cluster is processed before its parent.
    void sortClusters();

    /// \brief Merge the cluster and valid instance.
    void mergeCluster();

    /// \brief couples the cluster subtree with their parent trees in the first forest of the instance.
    void coupleSubtrees();

    /// \brief Check if the cluster root of the cluster at the given index was cutted
    /// and store it if that's the case.
    /// \param index of the cluster in the \ref cluster vector
    void collectCuttedClusterRoot(unsigned int index);

    /// \brief Cut all cluster terminals where the corresponding cluster roots have been cutted.
    /// \param index of the cluster in the \ref cluster vector
    void cutClusterTerminals(unsigned int index);

  public:
    explicit ClusterSolver(const std::shared_ptr<graph::Instance>& instance);

    ~ClusterSolver() override = default;

    bool solve() override;

    void unapplyReductions() override;

    /// \brief Provides the intended way to iterate over the cluster and solve them.
    /// \returns the cluster range, an iterable container that manages intermediate steps.
    ClusterRange& Clusters();
};

}  //namespace solver

#endif  //PACE2026_CLUSTER_SOLVER_HPP
