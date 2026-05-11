#include "AbstractSolver.hpp"

std::shared_ptr<graph::Instance>& solver::AbstractSolver::Instance()
{
    return this->instance;
}

const std::shared_ptr<graph::Instance>& solver::AbstractSolver::Instance() const
{
    return this->instance;
}

void solver::AbstractSolver::unapplyReductions()
{
    // default do nothing
}