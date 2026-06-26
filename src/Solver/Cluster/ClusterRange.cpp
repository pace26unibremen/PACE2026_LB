#include "ClusterRange.hpp"

#include "ClusterSolver.hpp"

solver::ClusterRange::ClusterRange(solver::ClusterSolver* solver) :
        solver(solver)
{}

solver::ClusterRange::ClusterRangeIterator solver::ClusterRange::begin()
{
    return {0, solver};
}

solver::ClusterRange::ClusterRangeIterator solver::ClusterRange::end()
{
    return {(unsigned int) solver->cluster.size(), solver};
}

solver::ClusterRange::ClusterRangeIterator::ClusterRangeIterator(const unsigned int currentIndex, ClusterSolver* solver) :
        currentIndex(currentIndex),
        solver(solver)
{}

solver::ClusterRange::ClusterRangeIterator::value_type solver::ClusterRange::ClusterRangeIterator::operator*() const
{
    auto context = std::make_shared<Context>();
    auto cluster = solver->cluster.at(currentIndex);

    // add the label of the cluster root the context
    if (unsigned int clusterRootLabel = solver->clusterToRootLabel.at(cluster); clusterRootLabel != 0)
    {
        context->clusterRootLabel = clusterRootLabel;
    }

    return {cluster, context};
}

solver::ClusterRange::ClusterRangeIterator& solver::ClusterRange::ClusterRangeIterator::operator++()
{
    // check if the cluster root in the last cluster is cutted
    solver->collectCuttedClusterRoot(currentIndex);
    // step forward
    currentIndex++;
    if (currentIndex == solver->cluster.size())
        return *this;
    // cut the cluster terminals in the current cluster,
    // for which the corresponding cluster root was cutted in the previous clusters
    solver->cutClusterTerminals(currentIndex);
    return *this;
}

solver::ClusterRange::ClusterRangeIterator solver::ClusterRange::ClusterRangeIterator::operator++(int)
{
    const ClusterRangeIterator tmp = *this;
    ++(*this);
    return tmp;
}

bool solver::ClusterRange::ClusterRangeIterator::operator==(const ClusterRangeIterator& other) const
{
    return currentIndex == other.currentIndex;
}

bool solver::ClusterRange::ClusterRangeIterator::operator!=(const ClusterRangeIterator& other) const
{
    return not(*this == other);
}
