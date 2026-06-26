#ifndef PACE2026_CLUSTER_RANGE_HPP
#define PACE2026_CLUSTER_RANGE_HPP

#include "../../Graph/Instance.hpp"
#include "../Context.hpp"

namespace solver
{

class ClusterSolver;

/// \brief
/// The cluster range is an <b>iterable</b> pseudo container for clusters,
/// that implements \c begin and \c end iterators. \ref ClusterRangeIterator
///
/// A ClusterRangeIterator provides the current cluster together with a pre-initialized context.
///
/// While iterating over the cluster range and solving one cluster after another
/// additional logic is executed to incorporate information from the previous cluster solution.
class ClusterRange
{
  private:
    /// \brief the underlying solver
    ClusterSolver* solver;

  public:
    /// \brief constructor
    /// \param solver underlying cluster solver
    explicit ClusterRange(ClusterSolver* solver);

    /// \brief The iterator class for the cluster range
    class ClusterRangeIterator
    {
      private:
        /// \brief the index of the current cluster
        unsigned int currentIndex;
        /// \brief the underlying solver
        ClusterSolver* solver;

      public:
        // define tags for STL function
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::pair<std::shared_ptr<graph::Instance>, std::shared_ptr<Context>>;

        ClusterRangeIterator(unsigned int currentIndex, ClusterSolver* solver);

        value_type operator*() const;

        ClusterRangeIterator& operator++();

        ClusterRangeIterator operator++(int);

        bool operator==(const ClusterRangeIterator& other) const;

        bool operator!=(const ClusterRangeIterator& other) const;
    };

    [[nodiscard]] ClusterRangeIterator begin();

    [[nodiscard]] ClusterRangeIterator end();
};

}  // namespace solver

#endif  //PACE2026_CLUSTER_RANGE_HPP
