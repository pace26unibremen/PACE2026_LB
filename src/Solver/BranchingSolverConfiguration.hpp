#ifndef PACE2026_BRANCHING_SOLVER_CONFIGURATION_HPP
#define PACE2026_BRANCHING_SOLVER_CONFIGURATION_HPP

#include "DebugPlugin.hpp"
#include "Rule/AbstractRule.hpp"
#include "Rule/CutBranchRule.hpp"
#include "Rule/EqualForestsRule.hpp"
#include "Rule/PairEqualRule.hpp"
#include "Rule/PairPathBranchingRule.hpp"
#include "Rule/PairUnconnectedBranchingRule.hpp"
#include "Rule/SingleVertexTreePropagationRule.hpp"
#include "Rule/DebugAssertFalseRule.hpp"

#include <functional>
#include <memory>

namespace solver
{
// forward declaration of \ref Context
struct Context;

/// \brief a function type that maps an instance and a context to a rule
using isApplicableFn = std::function<std::shared_ptr<AbstractRule>(const std::shared_ptr<graph::Instance>& instance,
                                                                   const std::shared_ptr<Context>& context)>;

/// \brief A struct that holds all configuration information and options for the \ref BranchingSolver.
struct BranchingSolverConfiguration
{
    /// \brief Whether the solver should perform a bounded depth search.
    /// I.e. the solver searches only for solutions that are equal to or better than a given parameter.
    /// If the solver doesn't find any suitable solution, it increases the parameter.
    /// The corresponding parameter is stored in the \ref Context as \ref Context::maxSolutionSize.
    bool boundedDephtSearch = true;

    /// \brief vector of the isApplicable function of rules.
    /// It defines which rules are used and in which order they are checked for applicability.
    std::vector<isApplicableFn> activeRules = {
        solver::CutBranchRule::isApplicable,
        solver::EqualForestsRule::isApplicable,
        solver::SingleVertexTreePropagationRule::isApplicable,
        solver::PairUnconnectedBranchingRule::isApplicable,
        solver::PairEqualRule::isApplicable,
        solver::PairPathBranchingRule::isApplicable,
        solver::DebugAssertFalseRule::isApplicable
        };

    /// \brief A debug plugin, nullptr for no additional debug info
    std::shared_ptr<DebugPlugin> debPlugin = nullptr;
};

} // namespace solver

#endif  //PACE2026_BRANCHING_SOLVER_CONFIGURATION_HPP
