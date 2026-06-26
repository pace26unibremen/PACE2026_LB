#ifndef PACE2026_FOREST_IO_HPP
#define PACE2026_FOREST_IO_HPP

#include "Forest.hpp"
#include "filesystem"
#include "iostream"
#include <string>

namespace graph
{

/// \brief A static class for IO operations on forests.
class ForestIO
{
  public:
    /// Reads a forest from input stream
    /// \param stream the input stream
    /// \param numberOfTerminals the number of leafs
    /// \param numberOfTrees the number of trees in the forest, default is 1
    /// \return a forest
    static Forest ReadNewick(std::istream& stream, int numberOfTerminals = 0, int numberOfTrees = 1);

    /// \brief Writes a forest to a stream.
    /// Writes in newick format.
    /// \param forest the instance to write
    /// \param stream the outstream
    static void WriteNewick(const Forest& forest, std::ostream& stream);

    /// \brief Writes a forest as dot graph to a stream.
    /// \param forest the instance to write
    /// \param stream the outstream
    static void WriteDot(const Forest& forest, std::ostream& stream);

    static void WriteDotMaxSAT(const Forest& forest, std::ostream& stream, 
                        const std::vector<int>& cutEdgeIndices,
                        const std::unordered_map<int, Node*>& indexToNode);

    /// \brief Writes a forest as dot graph to a stream.
    /// It does not construct a complete, valid dot file, but rather a subgraph/cluster within a dot file.
    /// \param forest the instance to write
    /// \param stream the outstream
    /// \param subgraphParams additional parameter to configure the subgraph in dot syntax. (e.g. "style=dotted;\n")
    /// \param verbose whether the dot graph contains address information
    static void WriteDotSubgraph(const Forest& forest,
                                 std::ostream& stream,
                                 std::string subgraphParams = "",
                                 bool verbose = false);
    
    static void WriteDotMaxSATSubgraph(const Forest& forest, std::ostream& stream, 
                                 std::string subgraphParams,
                                 const std::vector<int>& cutEdgeIndices,
                                 const std::unordered_map<int, Node*>& indexToNode);

};

}  // namespace graph

#endif  //PACE2026_FOREST_IO_HPP
