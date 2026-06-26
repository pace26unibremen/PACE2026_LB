#include "VisualizationPlugin.hpp"

#include "../../Graph/ForestIO.hpp"
#include "../Rule/AbstractBranchingRule.hpp"

#include <algorithm>
#include <utility>

solver::plugin::VisualizationPlugin::VisualizationPlugin(std::string dirPath)
{
    this->dirPath = std::move(dirPath);
    std::filesystem::create_directories(this->dirPath);
    overviewFile = std::ofstream(this->dirPath + "/overview.dot");
    if (!overviewFile.is_open())
    {
        throw std::invalid_argument("VisualizationPlugin : constructor : couldn't open file");
    }
}

void solver::plugin::VisualizationPlugin::writeStateNode()
{
    overviewFile << "state_" << stateIDs.top()
              << " [style=filled, "
                 "fillcolor=lightblue, "
                 "width=1,"
                 "label = \"" << stateIDs.top() << "\n";
    for (auto& f : *instance_)
    {
        overviewFile << f->Roots().size() << " ";
    }
    overviewFile << "\"];" << std::endl;
}

void solver::plugin::VisualizationPlugin::writeRuleNode(const std::shared_ptr<solver::AbstractRule>& rule)
{
    if (const auto branchingRule = std::dynamic_pointer_cast<solver::AbstractBranchingRule>(rule))
    {
        overviewFile << "subgraph rule_" << stateIDs.top() << " {\n"
        << "  cluster=true;\n"
        << "  width=3.5;\n"
        << "  label=\"" << branchingRule->name() << "\";\n"
        << "  labelloc=b;\n"
        << "  style=\"filled,rounded\";\n"
        << "  fillcolor=orange;\n";
        for (int branchId = 1; branchId <= branchingRule->MaxBranch(); branchId++)
        {
            overviewFile << "  rule_" << stateIDs.top() << "_" << branchId
                         << " [label=" << branchId << ", style=\"filled,dotted,rounded\", "
                                                      "fillcolor=darkorange, width=0.9, height=0.25, fixedsize=true];\n";
        }
        overviewFile << "};" << std::endl;
    }
    else
    {
        overviewFile << "rule_" << stateIDs.top()
        << R"( [style="filled,rounded", fillcolor=orange, width=3.5, label=")" << rule->name() << "\"]" << std::endl;
    }
}

void solver::plugin::VisualizationPlugin::dotInstance(const std::filesystem::path& path)
{
    std::ofstream os(path);
    if (!os.is_open())
    {
        throw std::invalid_argument("VisualizationPlugin : dotInstance : couldn't open file");
    }

    os << "digraph Instance {\n"
       << "splines = false\n\n";

    for (unsigned int i = 0; i < shadowInstance->size(); ++i)
    {
        std::shared_ptr<graph::Forest> forest = shadowInstance->at(i);
        // forests no longer in the active instance were removed by reductions — render them differently
        auto found = std::find(instance_->begin(), instance_->end(), forest) != instance_->end();

        std::string subgraphParam = (found ? "    style = dotted;\n" : "    style = \"dotted,filled\";\n");
        subgraphParam += "    cluster = true;\n"
                         "    node [\n"
                         "        shape = circle,\n"
                         "        fontsize = 15,\n"
                         "        label = \"\",\n"
                         "        height = 0.1,\n"
                         "        fillcolor = \"#00000022\",\n"
                         "        style = filled,\n"
                         "        fixedsize = true,\n"
                         "        labelloc = t];\n"
                         "    edge [arrowhead = none];\n\n";
        graph::ForestIO::WriteDotSubgraph(*forest, os, subgraphParam);
    }
    os << "}" << std::endl;
    os.close();
}

void solver::plugin::VisualizationPlugin::init(const std::shared_ptr<graph::Instance>& _instance,
                                               const std::shared_ptr<solver::Context>& _context)
{
    AbstractPlugin::init(_instance, _context);

    // keep a snapshot of the initial forests to track which ones get removed
    shadowInstance = std::make_shared<graph::Instance>();
    for (auto& f : *_instance) shadowInstance->push_back(f);

    overviewFile << "digraph overview {\n"
                 << "splines=ortho;\n"
                 << "ranksep=0.8;\n"
                 << "node [shape=box, margin=\"0.1,0.1\"];\n"
                 << "initial [label=\"\", shape=doublecircle, style=filled, fillcolor=green];\n";
    stateIDs.push(0);
    writeStateNode();
    overviewFile << "initial -> state_0 [URL=\"start.dot.svg\"]\n";
    dotInstance(dirPath + "/start.dot");
}

void solver::plugin::VisualizationPlugin::onApply(const std::shared_ptr<solver::AbstractRule>& rule)
{
    writeRuleNode(rule);
    maxStateId++;
    if (auto branchingRule = std::dynamic_pointer_cast<solver::AbstractBranchingRule>(rule))
    {
        overviewFile << "rule_" << stateIDs.top() << "_" << branchingRule->Branch()
                     << " -> state_" << maxStateId
                     << " [URL=\"rule_" << stateIDs.top() << "_" << branchingRule->Branch() << "_a.dot.svg\", target=F];\n";
        overviewFile << "state_" << stateIDs.top()
                     << " -> rule_" << stateIDs.top() << "_" << branchingRule->Branch()
                     << " [style=invis]";
        dotInstance(dirPath + "/rule_" + std::to_string(stateIDs.top()) +
            "_" + std::to_string(branchingRule->Branch()) + "_a.dot");
    }
    else
    {
        overviewFile << "rule_" << stateIDs.top()
                     << " -> state_" << maxStateId
                     << " [URL=\"rule_" << stateIDs.top() << "_a.dot.svg\", target=F];\n";
        overviewFile << "state_" << stateIDs.top()
                     << " -> rule_" << stateIDs.top()
                     << " [style=invis]";
        dotInstance(dirPath + "/rule_" + std::to_string(stateIDs.top()) + "_a.dot");
    }
    stateIDs.push(maxStateId);
    writeStateNode();
}

void solver::plugin::VisualizationPlugin::onUnapply(const std::shared_ptr<solver::AbstractRule>& rule)
{
    stateIDs.pop();
    if (auto branchingRule = std::dynamic_pointer_cast<solver::AbstractBranchingRule>(rule))
    {
        overviewFile << "state_" << stateIDs.top()
                     << " -> rule_" << stateIDs.top() << "_" << branchingRule->Branch()
                     << " [dir=back, URL=\"rule_" << stateIDs.top() << "_" << branchingRule->Branch() << "_u.dot.svg\", target=F];\n";
        dotInstance(dirPath + "/rule_" + std::to_string(stateIDs.top()) +
            "_" + std::to_string(branchingRule->Branch()) + "_u.dot");
    }
    else
    {
        overviewFile << "state_" << stateIDs.top()
                     << "-> rule_" << stateIDs.top()
                     << " [dir=back, URL=\"rule_" << stateIDs.top() << "_u.dot.svg\", target=F];\n";
        dotInstance(dirPath + "/rule_" + std::to_string(stateIDs.top()) + "_u.dot");
    }
}

void solver::plugin::VisualizationPlugin::onEnd()
{
    overviewFile << "}\n";
    overviewFile.close();
}

void solver::plugin::VisualizationPlugin::onReductionUnapply(const std::shared_ptr<solver::AbstractRule>& rule, bool lastRule)
{
    maxStateId++;
    if (lastRule)
    {
        overviewFile << "state_" << maxStateId << "[shape=doublecircle, label=\"\", style=filled, fillcolor=red];\n";
    }
    else
    {
        overviewFile << "state_" << maxStateId << "[shape=diamond, label=\"\", width=0.3, height=0.3];\n";
    }

    overviewFile << "state_" << stateIDs.top() << " -> "
                 << "state_" << maxStateId << "[URL=\"temp_" << stateIDs.top() << "_u.dot.svg\"];\n";
    dotInstance(dirPath + "/temp_" + std::to_string(stateIDs.top()) + "_u.dot");
    stateIDs.push(maxStateId);
}

void solver::plugin::VisualizationPlugin::onReductionReapply(const std::shared_ptr<solver::AbstractRule>& rule)
{
    int old = stateIDs.top();
    stateIDs.pop();
    overviewFile << "state_" << stateIDs.top() << " -> "
                 << "state_" << old << "[dir=back, URL=\"temp_" << stateIDs.top() << "_a.dot.svg\"];\n";
    dotInstance(dirPath + "/temp_" + std::to_string(stateIDs.top()) + "_a.dot");
}
