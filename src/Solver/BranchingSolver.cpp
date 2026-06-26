#include "BranchingSolver.hpp"

#include <algorithm>
#include <ranges>

solver::BranchingSolver::BranchingSolver(const std::shared_ptr<graph::Instance>& instance)
    : AbstractSolver(instance)
{
    this->context->branchingSolverConfiguration = this->configuration;
}

solver::BranchingSolver::BranchingSolver(const std::shared_ptr<graph::Instance>& instance,
                                         const std::shared_ptr<solver::BranchingSolverConfiguration>& configuration) :
    AbstractSolver(instance),
    configuration(configuration)
{
    this->context->branchingSolverConfiguration = this->configuration;
}

solver::BranchingSolver::BranchingSolver(const std::shared_ptr<graph::Instance>& instance,
                                         const std::shared_ptr<solver::BranchingSolverConfiguration>& configuration,
                                         const std::shared_ptr<solver::Context>& context) :
    AbstractSolver(instance),
    configuration(configuration),
    context(context)
{
    this->context->branchingSolverConfiguration = this->configuration;
}

void solver::BranchingSolver::setTimeoutFlag(std::atomic<bool>* flag)
{
    timeoutFlag = flag;
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

bool solver::BranchingSolver::solve()
{
    for (const auto& plugin : configuration->plugins) plugin->init(instance, context);

    // apply rules repeatedly until a return is triggerd
    while (true)
    {
        // On timeout, stop before starting a new iteration — but only once at
        // least one solution candidate has been found.  Without one, keep
        // searching so the solver always produces output within the grace period.
        if (timeoutFlag && timeoutFlag->load(std::memory_order_relaxed) && not solutionBranch.empty())
        {
            unwindAppliedRules();
            break;
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
                    for (const auto& plugin : configuration->plugins) plugin->onEnd();
                    return true;
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
                break;
            }
        }
    }

    // Reached when the search space is fully explored (unbounded depth) or when the
    // timeout flag fires. Write out the best solution found, if any.
    // solution may be nullptr when SIGTERM arrives before any candidate is found.

    // apply solution branch
    if (not solutionBranch.empty())
    {
        for (const auto& r : solutionBranch)
        {
            appliedRules.push_back(r);
            r->apply();
        }
    }
    for (const auto& plugin : configuration->plugins) plugin->onEnd();
    return not solutionBranch.empty();
}
