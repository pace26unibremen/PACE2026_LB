#include "ClusterSolver.hpp"

#include "../Context.hpp"
#include "../Rule/SingleVertexTreePropagationRule.hpp"
#include "ClusterPointGenerator.hpp"
#include "ClusterRange.hpp"

#include <algorithm>
#include <ranges>

std::shared_ptr<graph::Instance> solver::ClusterSolver::buildSingleCluster(unsigned int i)
{
    auto subInstance = std::make_shared<graph::Instance>();
    for (const auto& f : *instance)
    {
        graph::Node* root = f->Roots()[i];
        std::unordered_map<graph::Node*, unsigned int> mapTL = f->TerminalToLabel();
        std::unordered_map<unsigned int, graph::Node*> mapLT = f->LabelToTerminal();
        std::vector<graph::Node*> roots;

        roots.push_back(root);
        std::erase_if(mapTL, [root](const auto& entry) { return not root->hasTerminal(entry.second); });
        std::erase_if(mapLT, [root](const auto& entry) { return not root->hasTerminal(entry.first); });

        auto forest =
            std::make_shared<graph::Forest>(f->Nodes(),
                                            std::make_shared<std::unordered_map<graph::Node*, unsigned int>>(mapTL),
                                            std::make_shared<std::unordered_map<unsigned int, graph::Node*>>(mapLT),
                                            std::make_shared<std::vector<graph::Node*>>(roots));
        subInstance->push_back(forest);
    }
    return subInstance;
}

void solver::ClusterSolver::getClusterPoints()
{
    auto cpg = cluster::ClusterPointGenerator::wrappedConstructor(instance);

    for (auto clusterPointOfFrontForest : cpg.clusterPoints)
    {

        pointsAndForests_perCluster.push_back({{instance->at(0), clusterPointOfFrontForest}});

        // check to which forest the twin belongs
        for (auto twin : cpg.twinRelation.nodeToTwins[clusterPointOfFrontForest])
        {
            for (const auto& f : *instance)
            {
                const graph::Node* begin = f->Nodes()->data();
                const graph::Node* end = begin + f->Nodes()->size();
                if (twin >= begin && twin < end)
                {
                    pointsAndForests_perCluster.back().emplace_back(f, twin);
                    break;
                }
            }
        }
    }
}

void solver::ClusterSolver::resizeSubtreeTerminalsVector()
{
    auto maxLabel = std::ranges::max(instance->at(0)->LabelToTerminal() | std::ranges::views::keys);
    unsigned int numberOfNewLabels = 2 * pointsAndForests_perCluster.size();
    if ((maxLabel + numberOfNewLabels + 63) / 64 > (maxLabel + 63) / 64)
    {
        for (const auto& f : *instance)
        {
            for (auto& n : *f->Nodes())
            {
                n.subtreeTerminals.resize((maxLabel + numberOfNewLabels + 63) / 64);
            }
        }
    }
}

void solver::ClusterSolver::decoupleSubtrees()
{
    auto maxLabel = std::ranges::max(instance->at(0)->LabelToTerminal() | std::ranges::views::keys);
    for (const auto& _cluster : pointsAndForests_perCluster)
    {

        for (const auto& [f, n] : _cluster)
        {
            if (f == instance->at(0))
            {
                changesOnF0.emplace(f, n, maxLabel + 1, maxLabel + 2);
                changesOnF0.top().doAction();
            }
            else
            {
                changes.emplace(f, n, maxLabel + 1, maxLabel + 2);
                changes.top().doAction();
            }
        }

        clusterTerminal.insert(maxLabel + 1);
        clusterRoot.insert(maxLabel + 2);

        maxLabel += 2;
    }
}

void solver::ClusterSolver::splitInstance()
{
    for (unsigned int i = 0; i < instance->at(0)->Roots().size(); ++i)
    {
        auto c = buildSingleCluster(i);
        cluster.push_back(c);

        // fill maps rootLabelToCluster, clusterToRootLabel, clusterToClusterLabels
        bool isRootCluster = true;
        for (unsigned int l : clusterRoot)
        {
            if (c->at(0)->LabelToTerminal().contains(l))
            {
                clusterToRootLabel.insert({c, l});
                rootLabelToCluster.insert({l, c});
                isRootCluster = false;
                break;
            }
        }
        if (isRootCluster)
        {
            clusterToRootLabel.insert({c, 0});
            rootLabelToCluster.insert({0, c});
        }
        clusterToClusterLabels.insert({c, {}});
        for (unsigned int l : clusterTerminal)
        {
            if (c->at(0)->LabelToTerminal().contains(l))
            {
                clusterToClusterLabels.at(c).insert(l);
            }
        }
    }
}

void solver::ClusterSolver::sortClusters()
{
    // Post-order DFS: children are pushed before their parent,
    // ensuring bottom-up processing order in \ref cluster.
    std::function<void(std::shared_ptr<graph::Instance>, std::vector<std::shared_ptr<graph::Instance>>&)> dfs =
        [this, &dfs](std::shared_ptr<graph::Instance> c, std::vector<std::shared_ptr<graph::Instance>>& r) {
            for (auto childClusterLabel : clusterToClusterLabels.at(c))
            {
                auto childCluster = rootLabelToCluster.at(childClusterLabel + 1);
                dfs(childCluster, r);
            }
            r.push_back(c);
        };

    std::vector<std::shared_ptr<graph::Instance>> sorted;
    auto rootCluster = rootLabelToCluster.at(0);
    dfs(rootCluster, sorted);
    cluster = sorted;
}

void solver::ClusterSolver::mergeCluster()
{
    // we only need the first forest
    instance->erase(instance->begin() + 1, instance->end());
    instance->at(0)->Roots().clear();
    for (auto& c : cluster)
    {
        instance->at(0)->Roots().insert(instance->at(0)->Roots().end(), c->at(0)->Roots().begin(),
                                        c->at(0)->Roots().end());
    }
    std::ranges::sort(instance->at(0)->Roots(),
                      [](const graph::Node* r1, const graph::Node* r2) { return r1->hasSmallestTerminal(r2); });
}

void solver::ClusterSolver::coupleSubtrees()
{
    while (not changesOnF0.empty())
    {
        changesOnF0.top().undoAction();
        changesOnF0.pop();
    }
}

void solver::ClusterSolver::collectCuttedClusterRoot(unsigned int index)
{
    auto c = cluster.at(index);
    auto clusterRootLabel = clusterToRootLabel.at(c);
    if (clusterRootLabel != 0)
    {
        auto clusterRootNode = c->at(0)->LabelToTerminal().at(clusterRootLabel);
        if (clusterRootNode->isTrueTerminal() and not clusterRootNode->parent)
        {
            cuttedClusterRoots.insert(clusterRootLabel);
        }
    }
}

void solver::ClusterSolver::cutClusterTerminals(unsigned int index)
{
    auto c = cluster.at(index);

    std::unordered_set<unsigned int> cutClusterTerminals;
    for (auto l : clusterToClusterLabels.at(c))
    {
        if (cuttedClusterRoots.contains(l + 1))
        {
            cutClusterTerminals.insert(l);
        }
    }
    if (not cutClusterTerminals.empty())
    {
        auto r = solver::SingleVertexTreePropagationRule(c, std::make_shared<Context>(), cutClusterTerminals);
        r.apply();
    }
}

solver::ClusterSolver::ClusterSolver(const std::shared_ptr<graph::Instance>& instance) :
        AbstractSolver(instance),
        clusterRange(std::make_shared<solver::ClusterRange>(this))
{}

bool solver::ClusterSolver::solve()
{
    getClusterPoints();
    if (not pointsAndForests_perCluster.empty())
    {
        resizeSubtreeTerminalsVector();
        decoupleSubtrees();
        splitInstance();
        sortClusters();
    }
    // If there are no clusters, create a dummy cluster, so the instance can be solved on that.
    else
    {
        cluster = {instance};
        clusterToRootLabel.insert({instance, 0});
        rootLabelToCluster.insert({0, instance});
        clusterToClusterLabels.insert({instance, {}});
    }
    return false;
}

void solver::ClusterSolver::unapplyReductions()
{
    if (not pointsAndForests_perCluster.empty())
    {
        mergeCluster();
        coupleSubtrees();
    }
}

solver::ClusterRange& solver::ClusterSolver::Clusters()
{
    return *clusterRange.get();
}
