#include "ILPConstraintUtils.hpp"
#include "TreeUtils.hpp"

#include <cassert>

namespace solver {

std::vector<std::set<int>> computeTripleConstraints(
                const graph::Forest& forest1,
                const graph::Forest& forest2,
                cluster::LeastCommonAncestor& lca1,
                cluster::LeastCommonAncestor& lca2,
                const std::unordered_map<const graph::Node*, int>& nodeToIndex1,
                const std::unordered_map<const graph::Node*, int>& nodeToIndex2)
{
    // Both Forests should have the same (number) of leaves...
    assert(forest1.LabelToTerminal().size() == forest2.LabelToTerminal().size());

    // Both Index-Maps should have the same size as the Nodes()...
    assert(nodeToIndex1.size() == forest1.Nodes().size());
    assert(nodeToIndex2.size() == forest2.Nodes().size());

    std::vector<std::set<int>> tripleConstraints;
    int numLeaves = forest1.LabelToTerminal().size();

    for(unsigned int i = 1; i <= numLeaves; ++i)
    {
        for(unsigned int j = i+1; j <= numLeaves; ++j)
        {
            for(unsigned int k = j+1; k <= numLeaves; ++k)
            {
                bool fIncTriple = checkIncompatibleTriple(i, j, k, forest1, forest2, lca1, lca2, nodeToIndex1, nodeToIndex2);
                
                // If Triple is compatible just continue...
                if (!fIncTriple) continue;

                std::set<int> edgesij = getPath(forest1, i, j, lca1, nodeToIndex1);
                std::set<int> edgesjk = getPath(forest1, j, k, lca1, nodeToIndex1);
                std::set<int> edgesik = getPath(forest1, i, k, lca1, nodeToIndex1);

                // Union aller drei Pfade
                std::set<int> triPathEdges = edgesij;
                triPathEdges.insert(edgesjk.begin(), edgesjk.end());
                triPathEdges.insert(edgesik.begin(), edgesik.end());

                tripleConstraints.push_back(triPathEdges);

            }
        }
    }

    return tripleConstraints;
}

std::vector<std::set<int>> computePathPairConstraints(
                const graph::Forest& forest1,
                const graph::Forest& forest2,
                cluster::LeastCommonAncestor& lca1,
                cluster::LeastCommonAncestor& lca2,
                const std::unordered_map<const graph::Node*, int>& nodeToIndex1,
                const std::unordered_map<const graph::Node*, int>& nodeToIndex2)
{
    // Both Forests should have the same (number) of leaves...
    assert(forest1.LabelToTerminal().size() == forest2.LabelToTerminal().size());

    // Both Index-Maps should have the same size as the Nodes()...
    assert(nodeToIndex1.size() == forest1.Nodes().size());
    assert(nodeToIndex2.size() == forest2.Nodes().size());

    std::vector<std::set<int>> pathPairConstraints;
    int numLeaves = forest1.LabelToTerminal().size();

    for(unsigned int i = 1; i <= numLeaves; ++i)
    {
        for(unsigned int j = i+1; j <= numLeaves; ++j)
        {
            for(unsigned int p = i+1; p <= numLeaves; ++p)
            {
                // Only look at unique leaf pairs...
                if(p == j) continue;
                for(unsigned int q = p+1; q <= numLeaves; ++q)
                {
                    // Only look at unique leaf pairs...
                    if(q == j) continue;

                    // Only Important case is, if paths of pairs are disjoint in forest1
                    // but not disjoint in forest2...
                    if(!areTwoPathsDisjoint(forest1, i, j, p, q, lca1, nodeToIndex1)) continue;
                    if( areTwoPathsDisjoint(forest2, i, j, p, q, lca2, nodeToIndex2)) continue;

                    std::set<int> edgesij = getPath(forest1, i, j, lca1, nodeToIndex1);
                    std::set<int> edgespq = getPath(forest1, p, q, lca1, nodeToIndex1);

                    std::set<int> pathPairEdges = edgesij;
                    pathPairEdges.insert(edgespq.begin(), edgespq.end());

                    pathPairConstraints.push_back(pathPairEdges);
                }
            }
        }
    }

    return pathPairConstraints;
}


} // namespace solver