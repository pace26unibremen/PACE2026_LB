#include "TrivialSolver.hpp"

solver::TrivialSolver::TrivialSolver(const std::shared_ptr<graph::Instance>& instance)
    : AbstractSolver(instance)
{}

bool solver::TrivialSolver::solve()
{
    unsigned int numberOfTerminals = instance->at(0)->TerminalToLabel().size();

    auto nodes = std::make_shared<std::vector<graph::Node>>(numberOfTerminals);
    auto roots = std::make_shared<std::vector<graph::Node*>>(numberOfTerminals);
    auto terminalToLabel = std::make_shared<std::unordered_map<graph::Node*, unsigned int>>(numberOfTerminals);
    auto labelToTerminal = std::make_shared<std::unordered_map<unsigned int, graph::Node*>>(numberOfTerminals);

    int index = 0;
    for(const auto & [_, label] : instance->at(0)->TerminalToLabel())
    {
        graph::Node& node = nodes->at(index);
        node= {nullptr,nullptr,nullptr,nullptr};
        roots->at(index) = &node;
        terminalToLabel->emplace(&node,label);
        labelToTerminal->emplace(label,&node);
        index++;
    }
    *instance = {std::make_shared<graph::Forest>(nodes, terminalToLabel, labelToTerminal, roots)};

    return true;
}