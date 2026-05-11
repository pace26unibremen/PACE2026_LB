#ifndef PACE2026_TRIVIAL_SOLVER_HPP
#define PACE2026_TRIVIAL_SOLVER_HPP

#include "AbstractSolver.hpp"

namespace solver
{

/// \brief A Solver that returns the terminals as agreement forest
class TrivialSolver : AbstractSolver
{
  public:
    explicit TrivialSolver(const std::shared_ptr<graph::Instance>& instance);

    ~TrivialSolver() override = default;

    bool solve() override;
};

}  //namespace solver

#endif  //PACE2026_TRIVIAL_SOLVER_HPP
