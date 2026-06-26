#include "ForestIO.hpp"

#include <fstream>
#include <stack>
#include <unordered_map>
#include <vector>
#include <functional>
#include <stdexcept>

using namespace std;
using namespace graph;

Forest ForestIO::ReadNewick(std::istream& stream, int numberOfTerminals, int numberOfTrees)
{
    auto terminalToLabel = make_shared<unordered_map<Node*, unsigned int>>();
    auto labelToTerminal = make_shared<unordered_map<unsigned int, Node*>>();
    auto nodes = make_shared<vector<Node>>();
    auto roots = make_shared<vector<Node*>>();

    // IMPORTANT for pointer stability:
    // emplace_back() can reallocate the vector; that would invalidate Node* pointers.
    // If you can estimate an upper bound (like 2*n-1 for a binary tree), reserve the maximum needed capacity.
    if(numberOfTerminals > 0)
    {
        nodes->reserve(2* numberOfTerminals - 1);
        terminalToLabel->reserve(numberOfTerminals);
        labelToTerminal->reserve(numberOfTerminals);
        roots->reserve(numberOfTerminals);
    }
    else
    {
        throw invalid_argument("ForestIO : ReadNewick : number of terminals must be positive");
    }

    // Stack of "currently open" internal nodes:
    // When we see '(' we create an internal node and push it.
    // When we see ')' we close that subtree and pop it.
    stack<Node*> parentStack;

    // Tracks the previously created child under the current parent, so we can connect siblings correctly.
    Node* sibling = nullptr;

    for(int treeCounter = 0; treeCounter < numberOfTrees; treeCounter++)
    {
        string line;

        // First node we create for this tree will be its root
        Node* currentRoot = nullptr;

        // Helper function to create a new node and link it to the current parent and sibling.
        std::function<graph::Node*(void)> addNode = [&](){
            // Create new internal node
            nodes->emplace_back();
            Node* terminal = &nodes->back();

            // Link to parent (if any)
            if (not parentStack.empty())
            {
                Node* parent = parentStack.top();
                terminal->parent = parent;

                // Attach as left child if it's the first child,
                // otherwise as right child and set sibling links.
                if (sibling == nullptr)
                {
                    parent->leftChild = terminal;
                }
                else
                {
                    parent->rightChild = terminal;
                    sibling->sibling = terminal;
                    terminal->sibling = sibling;
                }
            }

            // If the root is not yet assigned, this is the first node -> root
            if (currentRoot == nullptr)
                currentRoot = terminal;

            return terminal;
        };

        while (getline(stream, line))
        {
            // Skip empty lines and comments
            if (line.empty())
                continue;
            if (line[0] == '#')
                continue;

            std::string label;

            // Parse the Newick format line
            for (const char& c : line)
            {
                switch (c)
                {
                    // End of tree
                    case ';':
                    {
                        // If there is a pending label right before ';', emit that leaf.
                        if (not label.empty())
                        {
                            Node* terminal = addNode();

                            // This is a terminal node, so we need to record its label.
                            unsigned int lab = static_cast<unsigned int>(std::stoi(label));
                            terminalToLabel->emplace(terminal, lab);
                            labelToTerminal->emplace(lab, terminal);
                            label.clear();
                        }

                        goto endTree;
                    }

                    // Separator between two children
                    case ',':
                    {
                        // If there is a pending terminal label before ',', emit that terminal first.
                        if (not label.empty())
                        {
                            Node* terminal = addNode();

                            // This terminal becomes the "previous child" before the comma,
                            // so the next child can link sibling pointers to it.
                            sibling = terminal;

                            // This is a terminal node, so we need to record its label.
                            unsigned int lab = static_cast<unsigned int>(std::stoi(label));
                            terminalToLabel->emplace(terminal, lab);
                            labelToTerminal->emplace(lab, terminal);
                            label.clear();
                        }
                        break;
                    }

                    // End subtree
                    case ')':
                    {
                        // If there is a pending terminal label before ')', emit that terminal first.
                        if (not label.empty())
                        {
                            Node* terminal = addNode();

                            // This is a terminal node, so we need to record its label.
                            unsigned int lab = static_cast<unsigned int>(std::stoi(label));
                            terminalToLabel->emplace(terminal, lab);
                            labelToTerminal->emplace(lab, terminal);
                            label.clear();
                        }

                        // Close the current parent subtree
                        if (not parentStack.empty())
                        {
                            // After closing, the "sibling base" becomes the node we closed.
                            sibling = parentStack.top();
                            parentStack.pop();
                        }
                        else
                        {
                            sibling = nullptr;
                        }
                        break;
                    }

                    // Start subtree: create a new internal node.
                    case '(':
                    {
                        Node* terminal = addNode();

                        // We are entering a new child list under this new internal node,
                        // so reset sibling tracking and push this node as current parent.
                        sibling = nullptr;
                        parentStack.push(terminal);
                        break;
                    }

                    // Collect digits for terminal labels
                    default:
                        if (not isdigit(c))
                        {
                            throw invalid_argument("TreeIO : ReadNewick : unexpected symbol");
                        }
                        label += c;
                        break;
                }
            }
        }
        endTree:
        {
            // Prepare for the next tree

            // Finalize this tree: store its root pointer.
            if (currentRoot != nullptr)
                roots->push_back(currentRoot);
            else
                // If we never created any node for this tree, that means the input was invalid (e.g. empty line or just ';').
                throw invalid_argument("ForestIO : ReadNewick : no valid tree found in input");

            // Reset stacks and pointers for the next tree
            parentStack = stack<Node*>();
            sibling = nullptr;
        };
    }
    auto f =  Forest(nodes, terminalToLabel, labelToTerminal, roots);
    f.sortChildrenAndCollectTerminals();
    return f;
}

void ForestIO::WriteNewick(const Forest& forest, std::ostream& stream)
{
    for(Node* current : forest.Roots())
    {
        int down = true; // traversing tree structure downwards
        while (true)
        {
            if (down)
            {
                if (current->leftChild != nullptr)
                {
                    stream << "(";
                    current = current->leftChild;
                }
                else
                {
                    stream << forest.TerminalToLabel().at(current);
                    down = false;
                }
            }
            else
            {
                if (current->parent == nullptr) break;
                if (current == current->parent->leftChild)
                {
                    stream << ",";
                    current = current->sibling;
                    down = true;
                }
                else
                {
                    stream << ")";
                    current = current->parent;
                }
            }
        }
        stream << ";\n";
    }
}

void ForestIO::WriteDot(const Forest& forest, ostream& stream)
{
    stream << "digraph Tree {\n"
           << "splines = false;\n"
           << "bgcolor = transparent;\n\n";
    std::string subgraphParams =
        "style=invis;\n"
        "node [\n"
        "   shape = circle,\n"
        "   fontsize = 15,\n"
        "   label = \"\",\n"
        "   height = 0.1,\n"
        "   fillcolor = \"#00000022\",\n"
        "   style = filled,\n"
        "   fixedsize = true,\n"
        "   labelloc = t];\n"
        "edge [arrowhead = none];\n\n";

    WriteDotSubgraph(forest, stream, subgraphParams);

    stream << "}" << endl;
}

void ForestIO::WriteDotSubgraph(const Forest& forest, ostream& stream, std::string subgraphParams, bool verbose)
{
    stream << "subgraph forest_" << &forest << " {\n"
           << "cluster=true;\n"
           << subgraphParams << "\n";
    stream << "inv_" << &forest << " [style = invis];\n\n";

    if (verbose)
    {
        stream << "fAddr [shape = plaintext,label = " << "\"Forest Address " << &forest << "\", style=none];\n\n";
    }

    std::stack<Node*> siblings;
    for (auto t : forest.Roots())
    {
        auto current = t;
        while (current)
        {
            if (verbose)
            {
                if (forest.TerminalToLabel().contains(current))
                {
                    stream << "n" << current << " [label = \"" << forest.TerminalToLabel().at(current) << "\\n\\n\\n \","
                           <<" xlabel= \"" << current << "\"];\n";
                    stream << "n" << current << " -> inv_" << &forest << " [style = invis];\n";
                }
                else
                {
                    stream << "n" << current << " [xlabel= \"" << current << "\"];\n";
                }
            }
            else
            {
                if (forest.TerminalToLabel().contains(current))
                {
                    stream << "n" << current << " [label = \"" << forest.TerminalToLabel().at(current) << "\\n\\n\\n \"];\n";
                    stream << "n" << current << " -> inv_" << &forest << " [style = invis];\n";
                }
            }

            if (current->rightChild)
            {
                stream << "n" << current << " -> n" << current->rightChild << ";\n";
                siblings.push(current->rightChild);
            }
            if (current->leftChild)
            {
                stream << "n" << current << " -> n" << current->leftChild << ";\n";
                current = current->leftChild;
            }
            else
            {
                if (siblings.empty())
                {
                    current = nullptr;
                }
                else
                {
                    current = siblings.top();
                    siblings.pop();
                }
            }
        }
    }
    stream << "}" << endl;
}

void ForestIO::WriteDotMaxSAT(const Forest& forest, ostream& stream, 
                        const std::vector<int>& cutEdgeIndices,
                        const std::unordered_map<int, Node*>& indexToNode)
{
    stream << "digraph Tree {\n"
           << "splines = false\n\n";

    std::string subgraphParams =
        "style=invis;\n"
        "node [\n"
        "   shape = circle,\n"
        "   fontsize = 15,\n"
        "   label = \"\",\n"
        "   height = 0.1,\n"
        "   fillcolor = \"#00000022\",\n"
        "   style = filled,\n"
        "   fixedsize = true,\n"
        "   labelloc = t];\n"
        "edge [arrowhead = none];\n\n";

    WriteDotMaxSATSubgraph(forest, stream, subgraphParams, cutEdgeIndices, indexToNode);
    stream << "}" << endl;
}

void ForestIO::WriteDotMaxSATSubgraph(const Forest& forest, ostream& stream, 
                                 std::string subgraphParams,
                                 const std::vector<int>& cutEdgeIndices,
                                 const std::unordered_map<int, Node*>& indexToNode)
{
    // Set von Nodes die geschnitten werden sollen (für O(1) Lookup)
    std::set<Node*> cutNodes;
    for (int idx : cutEdgeIndices)
    {
        auto it = indexToNode.find(idx);
        if (it != indexToNode.end())
            cutNodes.insert(it->second);
    }

    stream << "subgraph forest_" << &forest << " {\n"
           << "cluster=true;\n"
           << subgraphParams << "\n";

    stream << "inv_" << &forest << " [style = invis];\n\n";

    std::stack<Node*> siblings;
    for (auto t : forest.Roots())
    {
        auto current = t;
        while (current)
        {
            if (forest.TerminalToLabel().contains(current))
            {
                stream << "n" << current << " [label = \"" << forest.TerminalToLabel().at(current) << "\\n\\n\\n \"];\n";
                stream << "n" << current << " -> inv_" << &forest << " [style = invis];\n";
            }

            if (current->rightChild)
            {
                bool isCut = cutNodes.count(current->rightChild) > 0;
                stream << "n" << current << " -> n" << current->rightChild;
                if (isCut)
                    stream << " [color=red, penwidth=3]";
                stream << ";\n";
                siblings.push(current->rightChild);
            }
            if (current->leftChild)
            {
                bool isCut = cutNodes.count(current->leftChild) > 0;
                stream << "n" << current << " -> n" << current->leftChild;
                if (isCut)
                    stream << " [color=red, penwidth=3]";
                stream << ";\n";
                current = current->leftChild;
            }
            else
            {
                if (siblings.empty())
                {
                    current = nullptr;
                }
                else
                {
                    current = siblings.top();
                    siblings.pop();
                }
            }
        }
    }
    stream << "}" << endl;
}