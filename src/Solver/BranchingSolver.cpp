#include "BranchingSolver.hpp"

#include <algorithm>
#include <ranges>

solver::BranchingSolver::BranchingSolver(const std::shared_ptr<graph::Instance>& instance)
    : AbstractSolver(instance)
{
    initializeContext();
}

solver::BranchingSolver::BranchingSolver(const std::shared_ptr<graph::Instance>& instance,
                                         const std::shared_ptr<solver::BranchingSolverConfiguration>& configuration) :
    AbstractSolver(instance),
    configuration(configuration)
{
    initializeContext();
}

solver::BranchingSolver::BranchingSolver(const std::shared_ptr<graph::Instance>& instance,
                                         const std::shared_ptr<solver::BranchingSolverConfiguration>& configuration,
                                         const std::shared_ptr<solver::Context>& context) :
    AbstractSolver(instance),
    configuration(configuration),
    context(context)
{
    initializeContext();
}

void solver::BranchingSolver::setTimeoutFlag(std::atomic<bool>* flag)
{
    timeoutFlag = flag;
}

void solver::BranchingSolver::setPauseDeadline(std::chrono::steady_clock::time_point deadline)
{
    this->pauseDeadline = deadline;
    this->pauseDeadlinePassed = false;
    this->clockCheckCountdown = 0;
}

bool solver::BranchingSolver::pauseReached() const
{
    if (pauseDeadlinePassed)
    {
        return true;
    }
    if (pauseDeadline == std::chrono::steady_clock::time_point::max())
    {
        return false;
    }
    if (clockCheckCountdown == 0)
    {
        clockCheckCountdown = kClockCheckStride;
        // Written as `not (now < deadline)` rather than `now >= deadline`: Global.h (force-included
        // for the UWrMaxSat backend) defines a generic operator>= for any T, which is ambiguous with
        // std::chrono::time_point's own operator>= (see IncrementalMAFSolver.cpp for the same idiom).
        pauseDeadlinePassed = not (std::chrono::steady_clock::now() < pauseDeadline);
        return pauseDeadlinePassed;
    }
    --clockCheckCountdown;
    return false;
}

void solver::BranchingSolver::seedSolution(std::list<std::shared_ptr<AbstractRule>> branch, float weight)
{
    solutionBranch = std::move(branch);
    context->bestSolutionWeight = weight;
}

void solver::BranchingSolver::unapplySolutionBranch()
{
    for (auto it = solutionBranch.rbegin(); it != solutionBranch.rend(); ++it)
    {
        (*it)->unapply();
    }
    solutionBranch.clear();
}

void solver::BranchingSolver::unwindAppliedRules()
{
    applyNext = std::queue<std::shared_ptr<AbstractRule>>();
    while (not appliedRules.empty())
    {
        auto rule = appliedRules.back();
        rule->unapply();
        for (const auto& plugin : configuration->plugins) plugin->onUnapply(rule);
        appliedRules.pop_back();
    }
}

bool solver::BranchingSolver::rollBackBranch()
{
    // Stop this branch on timeout — but only once at least one solution
    // candidate has been stored.  Without one, keep rolling back and exploring
    // until the first EndBranchWithSolutionCandidate so the solver always
    // produces output within the POSIX grace period.
    //
    // solutionBranch is replayed against the instance once solve() returns
    // (see below), so the currently in-progress branch must be fully unwound
    // first — otherwise the replay applies its cloned rules on top of a
    // half-cut, abandoned branch and corrupts the tree.
    if (timeoutFlag && timeoutFlag->load(std::memory_order_relaxed) && not solutionBranch.empty())
    {
        unwindAppliedRules();
        return true;
    }

    // all rule suggestions can be discarded at the end of a branch
    applyNext = std::queue<std::shared_ptr<AbstractRule>>();

    while (true)
    {
        if (appliedRules.empty())
        {
            return true;
        }

        auto rule = appliedRules.back();
        rule->unapply();
        for (const auto& plugin : configuration->plugins) plugin->onUnapply(rule);
        appliedRules.pop_back();

        if (auto branchingRule = std::dynamic_pointer_cast<AbstractBranchingRule>(rule))
        {
            if (not branchingRule->isFullyExplored())
            {
                // to enter the next branch, we have to apply the branching rule again
                applyNext.emplace(branchingRule);
                return false;
            }
        }
    }
}

void solver::BranchingSolver::checkSolutionCandidate()
{
    auto candidateWeight = context->weightFunction(instance->at(0));
    if (context->bestSolutionWeight > candidateWeight)
    {

        for (const auto& plugin : configuration->plugins)
            plugin->onNewBestSolution(candidateWeight);

        context->bestSolutionWeight = candidateWeight;
        auto branchCloneView = appliedRules | std::views::transform(
            [](const std::shared_ptr<AbstractRule>& r) { return r->clone(); });
        solutionBranch = {branchCloneView.begin(), branchCloneView.end()};
    }
}

void solver::BranchingSolver::initializeContext()
{
    // set configuration
    this->context->branchingSolverConfiguration = this->configuration;

    // define label order
    std::function<void(graph::Node*)> collectTerminalsDFS = [this, &collectTerminalsDFS](graph::Node* const & n)
    {
        if (n->leftChild)
        {
            collectTerminalsDFS(n->leftChild);
            collectTerminalsDFS(n->rightChild);
        }
        else
        {
            context->heuristicLabelOrder.push_back(this->instance->at(0)->TerminalToLabel().at(n));
        }
    };
    for (const auto& r : instance->at(0)->Roots())
    {
        collectTerminalsDFS(r);
    }
}

void solver::BranchingSolver::unapplyReductions()
{
    // The first reduction in appliedRules (forward) is the last one to be unapplied (reverse iteration).
    // Find it upfront so we can pass lastRule=true to plugins without a separate allocation.
    std::shared_ptr<AbstractRule> firstReduction;
    for (const auto& r : appliedRules)
        if (r->IsReduction()) { firstReduction = r; break; }

    // unapply all reduction rules to get solution for the original instance
    for (const auto& reductionRule : appliedRules | std::views::reverse
        | std::views::filter([](const std::shared_ptr<AbstractRule>& r){ return r->IsReduction();}))
    {
        reductionRule->unapply();
        for (const auto& plugin : configuration->plugins)
            plugin->onReductionUnapply(reductionRule, reductionRule == firstReduction);
    }
}

const std::shared_ptr<solver::Context>& solver::BranchingSolver::GetContext()
{
    return context;
}

solver::BranchingSolver::RunResult solver::BranchingSolver::advanceSearch()
{
    if (not searchStarted)
    {
        for (const auto& plugin : configuration->plugins) plugin->init(instance, context);
        searchStarted = true;
    }

    // apply rules repeatedly until a return is triggerd
    // Tracks whether this invocation has applied at least one rule yet. The coordinator re-arms the
    // pause deadline immediately before every call (even with an already-past deadline, e.g. to force
    // a prompt pause), so the pause check alone must not fire before any work has been done this call —
    // otherwise a persistently-expired deadline would livelock the search with zero forward progress
    // across calls. Certified/timeout are unaffected: they reflect stable state set once (not re-armed
    // every call), so honouring them on the very first iteration is safe and matches the pre-split solve().
    bool madeProgressThisCall = false;
    while (true)
    {
        // Certified early exit (lower-bound track): stop the moment the incumbent's size is within the
        // certified threshold floor(a*L)+b (L <= k*), a provably valid answer. Read straight from the
        // config flag and the current incumbent weight each iteration — no separate state to track. The
        // threshold defaults to -1, which no positive size meets, so this is inert unless armed. The seed
        // case is covered too: on the very first iteration the incumbent is the seeded approximation.
        const bool certified = configuration->certifiedEarlyExit && not solutionBranch.empty()
            && context->bestSolutionWeight <= static_cast<float>(context->certifiedThreshold);

        if (certified)
        {
            return RunResult::Solved;             // do NOT unwind; finalize() handles output
        }

        // On timeout, stop before starting a new iteration — but only once at
        // least one solution candidate has been found.  Without one, keep
        // searching so the solver always produces output within the grace period.
        if (timeoutFlag && timeoutFlag->load(std::memory_order_relaxed) && not solutionBranch.empty())
        {
            return RunResult::Solved;             // SIGTERM: treat as stop-with-solution
        }

        if (madeProgressThisCall && pauseReached())
        {
            return RunResult::Paused;             // preserve all state, resume later
        }

        std::shared_ptr<AbstractRule> rule = nullptr;

        // check if we have rules in the pipeline
        if (not applyNext.empty())
        {
            // take first rule of queue
            rule = applyNext.front();
            applyNext.pop();
        }
        else
        {
            // check the rules for applicability
            for (const auto& isApplicable : configuration->activeRules)
            {
                rule = isApplicable(instance, context);
                // take first applicable rule
                if (rule) break;
            }
        }

        for (const auto& plugin : configuration->plugins) plugin->beforeApply(rule);
        const auto returnCode = rule->apply();
        for (const auto& plugin : configuration->plugins) plugin->onApply(rule);
        appliedRules.push_back(rule);
        madeProgressThisCall = true;

        bool calculationFinished = false;
        switch (returnCode)
        {
            case RuleReturnCode::Continue:
                break;
            case RuleReturnCode::ContinueWithRuleSuggestion:
                for (const auto& r : *rule->NextRuleSuggestion())
                {
                    applyNext.emplace(r);
                }
                break;
            case RuleReturnCode::EndBranchWithSolutionCandidate:
                if (configuration->boundedDephtSearch)
                {
                    for (const auto& plugin : configuration->plugins) plugin->onBranchEnd();
                    // onEnd() is deferred to finalize() so it fires exactly once, whether
                    // solve() calls it immediately or a coordinator calls it later.
                    return RunResult::Solved;
                }
                else
                {
                    for (const auto& plugin : configuration->plugins) plugin->onBranchEnd();
                    checkSolutionCandidate();
                    calculationFinished = rollBackBranch();
                    break;
                }
            case RuleReturnCode::CutBranch:
                for (const auto& plugin : configuration->plugins) plugin->onBranchEnd();
                calculationFinished = rollBackBranch();
                break;
            case RuleReturnCode::ImidateReturn:
                calculationFinished = true;
                break;
            default:
                throw std::logic_error("BranchingSolver : solve : undefined return code rule " + rule->name());
        }

        if (calculationFinished)
        {
            if (configuration->boundedDephtSearch)
            {
                context->maxSolutionSize++;
            }
            else
            {
                return RunResult::Exhausted;
            }
        }
    }
}

void solver::BranchingSolver::finalize()
{
    // Reached when the search space is fully explored (unbounded depth), when the
    // timeout flag fires, or when the coordinator is done with a paused/certified search.
    // Unwind any half-explored in-progress branch first, then write out the best solution
    // found, if any. solution may be nullptr when SIGTERM arrives before any candidate is found.
    //
    // Bounded-depth mode never populates solutionBranch: its EndBranchWithSolutionCandidate return
    // in advanceSearch() leaves the solution materialised directly in the instance, with appliedRules
    // holding exactly the rules that produced it. Unwinding here would undo that solution, so this
    // mode skips straight to onEnd().
    if (not configuration->boundedDephtSearch)
    {
        unwindAppliedRules();
        if (not solutionBranch.empty())
        {
            for (const auto& r : solutionBranch)
            {
                appliedRules.push_back(r);
                r->apply();
            }
        }
    }
    for (const auto& plugin : configuration->plugins) plugin->onEnd();
}

bool solver::BranchingSolver::solve()
{
    // no pause deadline armed by default -> advanceSearch() only ever returns Solved or Exhausted here.
    const RunResult result = advanceSearch();
    finalize();

    // Bounded-depth mode never populates solutionBranch (see finalize()): its success is exactly
    // "advanceSearch() reported Solved", since that path materialises the answer directly in the
    // instance. Other modes keep the original contract of "did we ever record a solutionBranch".
    if (configuration->boundedDephtSearch)
    {
        return result == RunResult::Solved;
    }
    return not solutionBranch.empty();
}
