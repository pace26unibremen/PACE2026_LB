#ifndef PACE2026_CONTEXT_HPP
#define PACE2026_CONTEXT_HPP

#include "../Graph/Instance.hpp"

#include <cmath>
#include <functional>
#include <list>
#include <memory>
#include <unordered_set>

namespace solver
{
// forward declaration of \ref BranchingSolverConfiguration
struct BranchingSolverConfiguration;

/// \brief A context stores information about the instance and the state of the branching solver.
struct Context
{
    /// \brief The weight of the best solution, that the solver found so far.
    /// (i.e., this is mostly equal to the number of components in the agreement forest.)
    /// \see weightFunction
    /// Default value is +infinity
    float bestSolutionWeight = INFINITY;

    /// \brief The maximum size (number of trees) of the best solution, that the solver
    /// searches in a bounded depth search. \ref BranchingSolverConfiguration::boundedDephtSearch
    /// Default value is 1.
    unsigned int maxSolutionSize = 1;

    /// \brief The configuration of the branching solver.
    /// This should be set in the constructor of the branching solver.
    std::shared_ptr<BranchingSolverConfiguration> branchingSolverConfiguration = nullptr;

    /// \brief A set of all protected edges (edges that must never be cut).
    /// An edge is identified by the node it points to (the child-node).
    std::unordered_set<graph::Node*> protectedEdges = std::unordered_set<graph::Node*>();

    /// \brief The label of the terminal that annotates the root of a cluster.
    /// If the instance is not a cluster or is the root-cluster (and has no annotated root),
    /// then the label is 0.
    unsigned int clusterRootLabel = 0;

    std::function<float(std::shared_ptr<graph::Forest>)> weightFunction =
        [this](const std::shared_ptr<graph::Forest>& forest)
        {
            const float numberOfComponents = (float) forest->Roots().size();

            // We prefer a solution where the cluster root is a singelton
            // and this singleton is not a collapsed subtree.
            // We give an incentive for such solutions by subtractting
            // 0.5 from the weight.
            if (clusterRootLabel != 0)
            {
                auto rn = forest->LabelToTerminal()[clusterRootLabel];
                if (rn->parent == nullptr and rn->isTrueTerminal())
                {
                    return numberOfComponents - (float) 0.5;
                }
            }
            return numberOfComponents;
        };


    /// \brief A heuristic order of labels for solver to find applicable rules.
    /// It should be initialized by the solver.
    std::list<unsigned int> heuristicLabelOrder = {};
};

}  //namespace solver

#endif  //PACE2026_CONTEXT_HPP
