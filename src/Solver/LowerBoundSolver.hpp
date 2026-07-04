#ifndef PACE2026_LOWERBOUND_SOLVER_HPP
#define PACE2026_LOWERBOUND_SOLVER_HPP

#include "AbstractSolver.hpp"
#include "BranchingSolver.hpp"
#include "ILP/IncrementalMAFSolver.hpp"
#include "../Graph/Instance.hpp"
#include "Context.hpp"

#include <memory>

namespace solver
{

/// \brief The Branching Solver solves the MAF problem by repeatedly applying \ref AbstractRule "rules",
/// including \ref AbstractBranchingRule "branching rules", so that the solver's search space is a tree.
class LowerBoundSolver : public AbstractSolver
{
    private:
        /// \brief BranchingSolver for Upper Bound.
        std::shared_ptr<BranchingSolver> branchingSolver;
        /// \brief MaxSATSolver for Lower Bound.
        std::shared_ptr<IncrementalMAFSolver> maxsatSolver;

        /// \brief Context information about the instance and the solver state
        std::shared_ptr<Context> context = std::make_shared<Context>();

    public:
        /// \brief starts the solver
        /// \returns true if the solver solves the instance, else false
        bool solve() override;

        explicit LowerBoundSolver(const std::shared_ptr<graph::Instance>& instance,
                                  const std::shared_ptr<BranchingSolver>& branchingSolver);
        
        ~LowerBoundSolver() override = default;
};

} // namespace solver

#endif // PACE2026_LOWERBOUND_SOLVER_HPP