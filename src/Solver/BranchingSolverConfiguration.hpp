#ifndef PACE2026_BRANCHING_SOLVER_CONFIGURATION_HPP
#define PACE2026_BRANCHING_SOLVER_CONFIGURATION_HPP

#include "Plugin/AbstractPlugin.hpp"
#include "Rule/ABCBranchingRule.hpp"
#include "Rule/ACBranchingRule.hpp"
#include "Rule/AbstractRule.hpp"
#include "Rule/BRule.hpp"
#include "Rule/CheckSingleVertexTreesRule.hpp"
#include "Rule/CutBranchRule.hpp"
#include "Rule/DebugAssertFalseRule.hpp"
#include "Rule/EqualForestsRule.hpp"
#include "Rule/EqualPairReductionRule.hpp"
#include "Rule/ReverseBRule.hpp"
#include "Rule/SiblingRuleFactory.hpp"
#include "Rule/SingleVertexTreePropagationRule.hpp"
#include "Rule/TwoBRule.hpp"
#include "Rule/ChainReductionRule.hpp"

#include <functional>
#include <memory>
#include <vector>

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
    bool boundedDephtSearch = false;

    /// \brief Whether the search may stop early once its incumbent is certified valid (lower-bound track).
    /// When true, the solver stops and emits the incumbent as soon as its size is within the certified
    /// threshold \ref Context::certifiedThreshold (a provably valid answer). This is a *strategy* switch,
    /// deliberately separate from whether the instance carries "#a {a} {b}" constants: an instance may
    /// supply a/b yet still be solved with, say, a heuristic strategy that leaves this off. Only the
    /// lower-bound preset turns it on; default false leaves exact/heuristic search unchanged.
    bool certifiedEarlyExit = false;

    /// \brief vector of the isApplicable function of rules.
    /// It defines which rules are used and in which order they are checked for applicability.
    std::vector<isApplicableFn> activeRules = {
        solver::CutBranchRule::isApplicable,
        solver::CheckSingleVertexTreesRule::isApplicable,
        solver::SingleVertexTreePropagationRule::isApplicable,
        solver::SiblingRuleFactory::allRules,
        solver::DebugAssertFalseRule::isApplicable
        };

    /// \brief Plugins to run alongside the solver. Empty by default (no plugins active).
    /// Add plugins here to observe solver events — see \ref solver::plugin::AbstractPlugin.
    /// Example: plugins.push_back(std::make_shared<solver::plugin::VisualizationPlugin>("debugOutput/dot"));
    std::vector<std::shared_ptr<solver::plugin::AbstractPlugin>> plugins = {};

    /// \brief Preset for the approximation solver, valid for any number of trees.
    ///
    /// Runs a single linear path (none of these rules is an \ref AbstractBranchingRule) that
    /// constructs a valid agreement forest:
    ///  - \ref CheckSingleVertexTreesRule — termination check.
    ///  - \ref SingleVertexTreePropagationRule — free: mirror a leaf that became a singleton in
    ///    one forest into the others (paper Case 1).
    ///  - \ref EqualPairReductionRule — free: contract a cherry common to all forests (Case 2).
    ///  - \ref ForcedCherryCutRule — the sharp Whidden-Zeh 3-approximation cut, but only defined
    ///    for \c t == 2 (returns null otherwise). Kept first so two-tree instances get the better
    ///    ratio.
    ///  - \ref GreedyCherryCutRule — the general-\c t fallback: when a cherry conflicts, isolate one
    ///    of its leaves in every forest. No proven ratio, but always valid, so it makes the
    ///    approximation work for \c t > 2 (where ForcedCherryCutRule does not apply).
    ///  - \ref DebugAssertFalseRule — safety net; if it fires, a reachable state is uncovered by
    ///    the rules above (a real bug), because every forest with >=2 leaves has a cherry that is
    ///    either common (EqualPair) or conflicting (Forced/Greedy).
    ///
    /// Deliberately excludes SubtreeReductionRule/ChainReductionRule from this in-loop list; they
    /// are pure batching speed-ups applied once upfront in the pipeline's Reduction stage instead.
    static BranchingSolverConfiguration approximationRules()
    {
        BranchingSolverConfiguration c;
        c.boundedDephtSearch = false;
        c.activeRules = {
            solver::CheckSingleVertexTreesRule::isApplicable,
            solver::SingleVertexTreePropagationRule::isApplicable,
            // One dispatcher replaces EqualPairReductionRule + ForcedCherryCutRule + GreedyCherryCutRule:
            // it finds forest 0's cherry once, checks it against all forests once, and returns the right
            // rule — instead of three rules each independently re-scanning for the same cherry.
            solver::SiblingRuleFactory::approximationRule,
            solver::DebugAssertFalseRule::isApplicable
        };
        return c;
    }
};

} // namespace solver

#endif  //PACE2026_BRANCHING_SOLVER_CONFIGURATION_HPP
