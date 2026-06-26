#ifndef PACE2026_FOREST_HPP
#define PACE2026_FOREST_HPP

#include "Node.hpp"

#include <filesystem>
#include <unordered_map>
#include <vector>
#include <set>
#ifdef DEBUG_IMAGE_VIEW_GRAPH
#include <opencv2/core.hpp>
#endif

namespace graph
{

/// \brief A representation of a forest.
class Forest
{
  private:
    /// \brief Vector that stores all nodes of the forest.
    std::shared_ptr<std::vector<Node>> nodes;

    /// \brief A map that stores all pointers to terminals with corresponding terminal labels.
    std::shared_ptr<std::unordered_map<Node*, unsigned int>> terminalToLabel;

    /// \brief A map that stores all leaf labels with corresponding terminal Pointers.
    std::shared_ptr<std::unordered_map<unsigned int, Node*>> labelToTerminal;

    /// \brief Node pointers to root Nodes.
    std::shared_ptr<std::vector<Node*>> roots;

    /// \brief Checks consistency relations in triples of parent and both children
    /// for all triples in the subtree of parent.
    /// \param parent the root node of the subtree
    /// \param subtreeLabels reference to a map that maps nodes to it's leaf in the subtree,
    /// is filled during recursive descent
    /// \param nodes reference to a set of all visited nodes, is filled during recursive descent
    /// \param smallestTerminal reference the smallest, is updated during recursive descent
    /// \returns true if this triple and all triple in the subtree of parent have valid relations
    bool checkTriple(Node* parent,
                     std::unordered_map<Node*, unsigned int>& subtreeLabels,
                     std::set<Node*>& nodes,
                     unsigned int& smallestTerminal) const;

  public:

    #ifdef DEBUG_IMAGE_VIEW_GRAPH
    /// \brief a visualization of the forest for debug purpose
    cv::Mat image;
    /// \brief renders the visualization \ref image.
    void renderImage();
    #endif

    // ------------------------------------------------------------- //
    // ---- constructors ------------------------------------------- //
    // ------------------------------------------------------------- //

    /// \brief Constructor.
    Forest(std::shared_ptr<std::vector<Node>> nodes,
           std::shared_ptr<std::unordered_map<Node*, unsigned int>> terminalToLabel,
           std::shared_ptr<std::unordered_map<unsigned int, Node*>> labelToTerminal,
           std::shared_ptr<std::vector<Node*>> roots);

    /// \brief Constructor. Loads forest from a file in newick format.
    /// \param path to file
    /// \param numberOfTerminals number of leafs.
    /// \param numberOfTrees number of trees.
    [[maybe_unused]]
    explicit Forest(const std::filesystem::path& path, int numberOfTerminals, int numberOfTrees);

    /// \brief \b 1. Sorts the children of each node,
    /// such that the left child
    /// contains the minimum label of both children.\n
    /// And \b 2. fills \c subtreeTerminals for each node.
    void sortChildrenAndCollectTerminals();

    /// \brief Makes a deep copy of the forest.
    /// @return The copy
    [[nodiscard]]
    Forest copy() const;

    // ------------------------------------------------------------- //
    // ---- persistence -------------------------------------------- //
    // ------------------------------------------------------------- //

    /// \brief Writes forest to a stream in newick format.
    /// \param stream the outstream
    void write(std::ostream& stream) const;

    /// \brief Writes forest to a file in newick format.
    /// \param path to file
    [[maybe_unused]]
    void write(const std::string& path) const;

    /// \brief Writes forest as .dot graph to a stream.
    /// \param stream the outstream
    void dot(std::ostream& stream) const;
    

    /// \brief Writes forest as .dot graph to a file.
    /// \param path to file
    [[maybe_unused]]
    void dot(const std::string& path) const;
    
    void dotMaxSAT(std::ostream& stream, 
              const std::vector<int>& cutEdgeIndices,
              const std::unordered_map<int, Node*>& indexToNode) const;

    void dotMaxSAT(const std::string& path, 
                    const std::vector<int>& cutEdgeIndices,
                    const std::unordered_map<int, Node*>& indexToNode) const;

    // ------------------------------------------------------------- //
    // ---- access to member fields -------------------------------- //
    // ------------------------------------------------------------- //

    /// \brief Reference to nodes vector.
    /// \property Nodes
    [[nodiscard, maybe_unused]]
    std::shared_ptr<std::vector<Node>>& Nodes();

    /// \brief \c const reference to nodes vector.
    /// \property Nodes
    [[nodiscard, maybe_unused]]
    const std::shared_ptr<std::vector<Node>>& Nodes() const;

    /// \brief Reference terminals to label map.
    /// It maps a pointer of a terminal node to its label.
    /// If the node is associated with more than one label (due to collapsing)
    /// the map should return the smallest one.
    /// \property TerminalToLabel.
    [[nodiscard, maybe_unused]]
    std::unordered_map<Node*, unsigned int>& TerminalToLabel();

    /// \brief \c const reference to terminals to label map.
    /// It maps a pointer of a terminal node to its label.
    /// If the node is associated with more than one label (due to collapsing)
    /// the map should return the smallest one.
    /// \property TerminalToLabel.
    [[nodiscard, maybe_unused]]
    const std::unordered_map<Node*, unsigned int>& TerminalToLabel() const;

    /// \brief Reference to label to terminal map.
    /// It maps a label to the associated terminal pointer.
    /// \property LabelToTerminal.
    [[nodiscard, maybe_unused]]
    std::unordered_map<unsigned int, Node*>& LabelToTerminal();

    /// \brief \c const reference to label to terminal map.
    /// It maps a label to the associated terminal pointer.
    /// \property LabelToTerminal.
    [[nodiscard, maybe_unused]]
    const std::unordered_map<unsigned int, Node*>& LabelToTerminal() const;

    /// \brief Reference vector of root node pointer.
    /// \property Roots
    [[nodiscard, maybe_unused]]
    std::vector<Node*>& Roots();

    /// \brief \c const reference vector of root node pointer.
    /// \property Roots
    [[nodiscard, maybe_unused]]
    const std::vector<Node*>& Roots() const;

    /// \brief returns a pointer to the root node, that has \c node in its subtree
    /// \param node pointer to node
    /// \returns pointer to parent
    /// \throws logic_error if no parent was found
    [[nodiscard, maybe_unused]]
    Node* rootOf(Node* node) const;

    /// \brief Returns the maximum common X-Forest of this forest and the input forest.
    /// \param other the other forest to compare with.
    /// \returns list of node pointers to the root of the common subtrees in this forest
    std::vector<Node*> maximumCommonSubforestRoots(const Forest& other);

    /// \brief Checks whether the forest representation is valid
    /// and writes flaws to std::clog
    /// \returns true if the forest has a valid representation.
    bool isValid() const;

    /// \brief Operator that checks if the first tree is a subtree of the second.
    /// \param other The tree to compare with.
    /// \return true if the first tree is a subtree, false otherwise.
    bool isTrueSubtreeOf(const Forest& other) const;

    /// \brief checks for two nodes whether the underlying subtrees are identical
    /// \param subtree1 root node of the fist subtree
    /// \param subtree2 root node of the second subtree
    /// \returns true if the subtrees are identical.
    static bool hasIdenticalSubtree(Node* subtree1, Node* subtree2);

    // ------------------------------------------------------------- //
    // ---- operators ---------------------------------------------- //
    // ------------------------------------------------------------- //

    /// \brief Equality operator that compares two X-forest for structural equality.
    /// \param other the other forest to compare with.
    /// \return true if the forests are identical, false otherwise.
    bool operator==(const Forest& other) const;

};

}  // namespace graph

#endif  //PACE2026_FOREST_HPP
