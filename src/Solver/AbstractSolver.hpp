#ifndef PACE2026_ABSTRACT_SOLVER_HPP
#define PACE2026_ABSTRACT_SOLVER_HPP

#include "../Graph/Instance.hpp"
#include "../Graph/Forest.hpp"

namespace solver
{

/// \brief This is the base class for a solver of the MAF problem.
class AbstractSolver
{
  protected:
    /// \brief the instance to solve
    std::shared_ptr<graph::Instance> instance;

    /// \brief constructor
    /// \param instance to solve
    AbstractSolver(const std::shared_ptr<graph::Instance>& instance) : instance(instance) {};
  public:
    virtual ~AbstractSolver() = default;

    /// \brief solve the instance
    virtual bool solve() = 0;

    /// \brief Unapplies all reduction rules, that where applied to the instance.
    virtual void unapplyReductions();

    /// \brief reference to instance
    /// \property Instance
    [[nodiscard, maybe_unused]]
    std::shared_ptr<graph::Instance>& Instance();

    /// \brief \c const reference to instance
    /// \property Instance
    [[nodiscard, maybe_unused]]
    const std::shared_ptr<graph::Instance>& Instance() const;
};

} // namespace solver

#endif  //PACE2026_ABSTRACT_SOLVER_HPP
