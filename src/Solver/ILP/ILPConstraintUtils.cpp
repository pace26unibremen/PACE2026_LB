#include "ILPConstraintUtils.hpp"
#include "TreeUtils.hpp"

#include <cassert>
#include <iostream>
#include <algorithm>

namespace solver {

std::vector<std::vector<int>> computeTripleConstraints(const ForestData& forestData_1, const ForestData& forestData_2)
{
    std::vector<std::vector<int>> tripleConstraints;
    // Need the labels, due to reductions these may not be in order...
    std::vector<unsigned int> labels;
    for (const auto& [_, label] : forestData_1.forestPtr->TerminalToLabel())
    {
        labels.push_back(label);
    }
    std::sort(labels.begin(), labels.end());
    int numLeaves = labels.size();

    for(unsigned int i = 0; i < numLeaves; ++i)
    {
        for(unsigned int j = i+1; j < numLeaves; ++j)
        {
            for(unsigned int k = j+1; k < numLeaves; ++k)
            {
                bool fIncTriple = checkIncompatibleTriple(labels[i], labels[j], labels[k], forestData_1, forestData_2);
                
                // If Triple is compatible just continue...
                if (!fIncTriple) continue;

                std::vector<int> edgesij = getPath(labels[i], labels[j], forestData_1);
                std::vector<int> edgesjk = getPath(labels[j], labels[k], forestData_1);
                std::vector<int> edgesik = getPath(labels[i], labels[k], forestData_1);

                // Union of all paths...
                std::vector<int> triPathEdges;
                triPathEdges.reserve(edgesij.size() + edgesjk.size() + edgesik.size());
                triPathEdges.insert(triPathEdges.end(), edgesij.begin(), edgesij.end());
                triPathEdges.insert(triPathEdges.end(), edgesjk.begin(), edgesjk.end());
                triPathEdges.insert(triPathEdges.end(), edgesik.begin(), edgesik.end());
                
                // Make Sure it union of set is unique...
                std::sort(triPathEdges.begin(), triPathEdges.end());
                triPathEdges.erase(
                    std::unique(triPathEdges.begin(), triPathEdges.end()),
                    triPathEdges.end()
                );

                tripleConstraints.push_back(std::move(triPathEdges));

            }
        }
    }
    std::clog << "Number of Triple Constraints: " << tripleConstraints.size()  << std::endl;
    return tripleConstraints;
}

std::vector<std::vector<int>> computePathPairConstraints(const ForestData& forestData_1, const ForestData& forestData_2)
{
    std::vector<std::vector<int>> pathPairConstraints;

    // Need the labels, due to reductions these may not be iterating...
    std::vector<unsigned int> labels;
    for (const auto& [_, label] : forestData_1.forestPtr->TerminalToLabel())
    {
        labels.push_back(label);
    }
    std::sort(labels.begin(), labels.end());
    int numLeaves = labels.size();

    for(unsigned int i = 0; i < numLeaves; ++i)
    {
        for(unsigned int j = i+1; j < numLeaves; ++j)
        {
            for(unsigned int p = i+1; p < numLeaves; ++p)
            {
                // Only look at unique leaf pairs...
                if(p == j) continue;
                for(unsigned int q = p+1; q < numLeaves; ++q)
                {
                    // Only look at unique leaf pairs...
                    if(q == j) continue;

                    // but not disjoint in forest2...
                    if(!areTwoPathsDisjoint(labels[i], labels[j], labels[p], labels[q], forestData_1)) continue;
                    if( areTwoPathsDisjoint(labels[i], labels[j], labels[p], labels[q], forestData_2)) continue;
                    
                    std::vector<int> edgesij = getPath(labels[i], labels[j], forestData_1);
                    std::vector<int> edgespq = getPath(labels[p], labels[q], forestData_1);

                    std::vector<int> pathPairEdges;
                    pathPairEdges.reserve(edgesij.size() + edgespq.size());
                    pathPairEdges.insert(pathPairEdges.end(), edgesij.begin(), edgesij.end());
                    pathPairEdges.insert(pathPairEdges.end(), edgespq.begin(), edgespq.end());

                    std::sort(pathPairEdges.begin(), pathPairEdges.end());
                    pathPairEdges.erase(
                        std::unique(pathPairEdges.begin(), pathPairEdges.end()),
                        pathPairEdges.end()
                    );
                    pathPairConstraints.push_back(std::move(pathPairEdges));
                }
            }
        }
    }
    std::clog << "Number of PathPair Constraints: " << pathPairConstraints.size()  << std::endl;
    return pathPairConstraints;
}


} // namespace solver