
#include "Forest.hpp"

#include "ForestIO.hpp"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <ranges>
#include <stack>
#include <unordered_set>
#include <utility>

#ifdef DEBUG_IMAGE_VIEW_GRAPH
#include <graphviz/gvc.h>
#include <opencv2/imgcodecs.hpp>
#include <unistd.h>
#include <fcntl.h>
#include <sstream>
#endif

using namespace std;

namespace graph
{
// ------------------------------------------------------------- //
// ---- constructors ------------------------------------------- //
// ------------------------------------------------------------- //

Forest::Forest(std::shared_ptr<std::vector<Node>> nodes,
               std::shared_ptr<std::unordered_map<Node*, unsigned int>> terminalToLabel,
               std::shared_ptr<std::unordered_map<unsigned int, Node*>> labelToTerminal,
               std::shared_ptr<std::vector<Node*>> roots) :
        nodes(std::move(nodes)),
        terminalToLabel(std::move(terminalToLabel)),
        labelToTerminal(std::move(labelToTerminal)),
        roots(std::move(roots))
{
    sortChildrenAndCollectTerminals();

    #ifdef DEBUG_IMAGE_VIEW_GRAPH
    renderImage();
    #endif
}

Forest::Forest(const filesystem::path& path, int numberOfTerminals, int numberOfTrees)
{
    if (path.empty())
    {
        throw invalid_argument("Forest : Constructor : provided file path is empty");
    }
    ifstream file = ifstream(path);
    if (!file.is_open())
    {
        throw invalid_argument("Forest : Constructor : unable to open file");
    }
    *this = ForestIO::ReadNewick(file, numberOfTerminals, numberOfTrees);
}

// ------------------------------------------------------------- //
// ---- persistence -------------------------------------------- //
// ------------------------------------------------------------- //

void Forest::write(std::ostream& stream) const
{
    ForestIO::WriteNewick(*this, stream);
}

void Forest::write(const string& path) const
{
    std::ofstream outStream(path);
    if (!outStream.is_open())
    {
        throw std::invalid_argument("Forest : write : couldn't open file");
    }
    write(outStream);
    outStream.close();
}

void Forest::dot(ostream& stream) const
{
    ForestIO::WriteDot(*this, stream);
}

void Forest::dot(const string& path) const
{
    std::ofstream outStream(path);
    if (!outStream.is_open())
    {
        throw std::invalid_argument("Forest : dot : couldn't open file");
    }
    dot(outStream);
    outStream.close();
}

void Forest::dotMaxSAT(ostream& stream, 
                          const std::vector<int>& cutEdgeIndices,
                          const std::unordered_map<int, Node*>& indexToNode) const
{
    ForestIO::WriteDotMaxSAT(*this, stream, cutEdgeIndices, indexToNode);
}

void Forest::dotMaxSAT(const string& path, 
                          const std::vector<int>& cutEdgeIndices,
                          const std::unordered_map<int, Node*>& indexToNode) const
{
    std::ofstream outStream(path);
    if (!outStream.is_open())
    {
        throw std::invalid_argument("Forest : dotMaxSAT : couldn't open file");
    }
    dotMaxSAT(outStream, cutEdgeIndices, indexToNode);
    outStream.close();
}

// ------------------------------------------------------------- //
// ---- access to member fields -------------------------------- //
// ------------------------------------------------------------- //

vector<Node>& Forest::Nodes()
{
    return *this->nodes;
}

const vector<Node>& Forest::Nodes() const
{
    return *this->nodes;
}

std::unordered_map<Node*, unsigned int>& Forest::TerminalToLabel()
{
    return *this->terminalToLabel;
}

const std::unordered_map<Node*, unsigned int>& Forest::TerminalToLabel() const
{
    return *this->terminalToLabel;
}

std::unordered_map<unsigned int, Node*>& Forest::LabelToTerminal()
{
    return *this->labelToTerminal;
}

const std::unordered_map<unsigned int, Node*>& Forest::LabelToTerminal() const
{
    return *this->labelToTerminal;
}

std::vector<Node*>& Forest::Roots()
{
    return *this->roots;
}

const std::vector<Node*>& Forest::Roots() const
{
    return *this->roots;
}

Node* Forest::rootOf(Node* node) const
{
    for(auto rootPtr : *roots)
    {
        if(node->hasSubsetTerminals(rootPtr))
        {
            return rootPtr;
        }
    }
    throw std::logic_error("Forest : rootOf : no root found");
}

bool Forest::isValid() const
{
    bool valid = true;
    std::unordered_map<Node*, unsigned int> totalLeafs;
    unsigned int lastSmallestTerminal = 0;
    // Check all reachable Roots
    for (Node* rootPtr : *roots)
    {
        std::unordered_map<Node*, unsigned int> rootLeafs;
        unsigned int smallestTerminal = -1; // -> max value of unsigned int
        set<Node*> pointers;
        if (rootPtr->parent != nullptr)
        {
            std::clog << "Forest: isValid: Root has a parent:\n"
                         "   parent (" << rootPtr->parent << ") -> Root (" << rootPtr <<") \n"
                         "   Let's get to the root of the issue."<< endl;
            valid = false;
        }
        // Check children (recursive)
        valid &= checkTriple(rootPtr, rootLeafs, pointers, smallestTerminal);
        totalLeafs.merge(rootLeafs);
        // Check root order
        if (smallestTerminal < lastSmallestTerminal)
        {
            std::clog << "Forest: isValid: root order disrupted:\n"
                         "   root (" << rootPtr << ") -> smallestTerminal (" << smallestTerminal <<") \n"
                         "   previous smallest leaf (" << lastSmallestTerminal <<") \n"
                         "   Order in the court!"<< endl;
            valid = false;
        }
        lastSmallestTerminal = smallestTerminal;
    }
    // Check terminal -> label
    if (*terminalToLabel != totalLeafs)
    {
        std::clog << "Forest: isValid: unreachable leafs in terminalToLabel:\n"
                     "   List Leafs? \n"
                     "   She loves me. She loves me not. She loves me. She is undefined?"<< endl;
        valid = false;
    }

    return valid;
}

bool Forest::isTrueSubtreeOf(const Forest& other) const
{
    std::function<bool(Node*, Node*)> traverseUp = [&](const Node* thisNodePtr, const Node* otherNodePtr) -> bool {

        if (not thisNodePtr->hasSameTerminals(otherNodePtr))
        {
            return false;
        }
        if (thisNodePtr->parent == nullptr)
        {
            return true;
        }
        if (otherNodePtr->parent == nullptr)
        {
            return false;
        }
        return traverseUp(thisNodePtr->parent, otherNodePtr->parent);
    };

    if (this->labelToTerminal->size() > other.labelToTerminal->size())
    {
        return false;
    }

    for (auto& [key, value]: *labelToTerminal)
    {
        if (not traverseUp(value, other.labelToTerminal->at(key)))
        {
            return false;
        }
    }
    return true;
}

bool Forest::hasIdenticalSubtree(Node* subtree1, Node* subtree2)
{
    std::stack<Node*> visitingStack_subtree1;
    visitingStack_subtree1.push(subtree1);
    std::stack<Node*> visitingStack_subtree2;
    visitingStack_subtree2.push(subtree2);

    while (not (visitingStack_subtree1.empty() or visitingStack_subtree2.empty()))
    {
        const auto currentNode_subtree1 = visitingStack_subtree1.top();
        visitingStack_subtree1.pop();
        const auto currentNode_subtree2 = visitingStack_subtree2.top();
        visitingStack_subtree2.pop();

        if (not currentNode_subtree1->hasSameTerminals(currentNode_subtree2))
        {
            return false;
        }

        if (not (currentNode_subtree1->leftChild == nullptr or currentNode_subtree2->leftChild == nullptr))
        {
            // neither is a leaf node
            visitingStack_subtree1.push(currentNode_subtree1->rightChild);
            visitingStack_subtree1.push(currentNode_subtree1->leftChild);
            visitingStack_subtree2.push(currentNode_subtree2->rightChild);
            visitingStack_subtree2.push(currentNode_subtree2->leftChild);
        }
        else
        {
            // one or both are leaves
            if (not (currentNode_subtree1->leftChild == nullptr and currentNode_subtree2->rightChild == nullptr))
            {
                // just one is a leaf
                return false;
            }
        }
    }
    if (visitingStack_subtree1.empty() and visitingStack_subtree2.empty())
    {
        return true;
    }
    return false;
}

bool Forest::checkTriple(
    Node* parent,
    std::unordered_map<Node*, unsigned int>& subtreeLabels,
    set<Node*>& nodes,
    unsigned int& smallestTerminal) const
{
    bool tripleValid = true;
    const auto& left = parent->leftChild;
    const auto& right = parent->rightChild;

    // check if node was already visited
    if (!nodes.insert(parent).second)
    {
        std::clog << "Forest: checkTriple: Node appears second time in tree:\n"
                         "   Pointer (" << parent << ") \n"
                         "   Who is the original?"<< endl;
        tripleValid = false;
    }

    // check the different cases for which children exist
    if (left == nullptr && right == nullptr)
    {
        // case parent is a terminal
        if (terminalToLabel->contains(parent))
        {
            unsigned int label = terminalToLabel->at(parent);
            // check 'label -> parent' fits to 'parent -> label'
            if (labelToTerminal->at(label) != parent)
            {
                std::clog << "Forest: checkTriple: label to terminal incorrect:\n"
                         "   Node (" << parent << ") -> Label ("<< label <<")\n"
                         "   Label ("<< label <<") -> Node (" << labelToTerminal->at(label) << ")\n"
                         "   Is this unrequited love?"<< endl;
                tripleValid = false;
            }

            // update the smallest terminal in tree
            if (label < smallestTerminal)
            {
                smallestTerminal = label;
            }

            // update
            subtreeLabels.emplace(parent, label);
        } else
        {
            // parent has no children, but is not a terminal
            std::clog << "Forest: checkTriple: Leaf has no label:\n"
                         "   Node (" << parent << ") \n"
                         "   Maybe he can sign with Sony?"<< endl;
            tripleValid = false;
        }
    }
    else if (left != nullptr && right != nullptr)
    {
        // case parent is an inner node
        // for clang

        // Order
        if (right->hasSmallestTerminal(left))
        {
            std::clog << "Forest: checkTriple: unordered Tree:\n"
                         "   parent (" << parent << ") -> (" << parent->leftChild <<") , (" << parent->rightChild <<") \n"
                         "   Commander Cody, the time has come. Execute Order 66."<< endl;
            tripleValid = false;
        }
        // left child
        //cCheck parent
        if (left->parent != parent)
        {
            std::clog << "Forest: checkTriple: left child forgot his parent:\n"
                         "   parent ("
                      << parent << ") -> (" << left
                      << ") child\n"
                         "   parent ("
                      << left->parent << ") <- (" << left
                      << ") child\n"
                         "   Why bother raising them if they forget about you?"
                      << endl;
            tripleValid = false;
        }
        // check sibling
        if (left->sibling != right)
        {
            std::clog << "Forest: checkTriple: left child forgot his sibling:\n"
                         "   parent ("
                      << parent << ") -> (" << left << " and " << right
                      << ") children\n"
                         "   first child ("
                      << left << ") -> (" << left->sibling
                      << ") sibling"
                         "   Maybe they had a fight?"
                      << endl;
            tripleValid = false;
        }
        // right child
        // check parent
        if (right->parent != parent)
        {
            std::clog << "Forest: checkTriple: right child forgot his parent:\n"
                         "   parent ("
                      << parent << ") -> (" << right
                      << ") child\n"
                         "   parent ("
                      << right->parent << ") <- (" << right
                      << ") child\n"
                         "   Why bother raising them if they forget about you?"
                      << endl;
            tripleValid = false;
        }
        // check sibling
        if (right->sibling != left)
        {
            std::clog << "Forest: checkTriple: right child forgot his sibling:\n"
                         "   parent ("
                      << parent << ") -> (" << left << " and " << right
                      << ") children\n"
                         "   first child ("
                      << right << ") -> (" << right->sibling
                      << ") sibling"
                         "   Maybe they had a fight?"
                      << endl;
            tripleValid = false;
        }

        // Recursive call with children
        std::unordered_map<Node*, unsigned int> leftLabels;
        std::unordered_map<Node*, unsigned int> rightLabels;
        tripleValid &= checkTriple(left, leftLabels, nodes,smallestTerminal) &&
                       checkTriple(right, rightLabels, nodes,smallestTerminal);
        leftLabels.merge(rightLabels);
        // collect labels of subtree

        // overwriting subtree labels reference
        // (there could be stuff from another part of tree, because it's a reference ...
        // the reference behavior is only used at the initial call not within the recursion)
        subtreeLabels = leftLabels;
    }
    else
    {
        // case parent has only one child
        std::clog << "Forest: checkTriple: unbalanced Tree:\n"
                     "   parent (" << parent << ") -> (" << parent->leftChild <<") , (" << parent->rightChild <<") \n"
                     "   Parent needs to get active and produce another child."<< endl;
        tripleValid = false;
        // for the sake of completeness, we should also check the subtree of the one child ...
    }


    // Convert found leafs into uint vector to compare
    auto foundTerminals = std::vector<uint64_t>((subtreeLabels.size() + 63) / 64);
    for (const auto& label : subtreeLabels | views::values)
    {
        constexpr uint64_t one = 1;
        foundTerminals[(label - 1) / 64] += (one<<(label - 1) % 64);
    }

    // Compare found terminals with terminals saved in node
    if (foundTerminals != parent->subtreeTerminals)
    {
        std::clog << "Forest: checkTriple: subtreeTerminals list is incorrect:\n"
                        "   at pointer (" << parent << ") \n"
                        "   Maybe you should fix that? ;)"<< endl;
        tripleValid = false;
    }
    return tripleValid;
}

// ------------------------------------------------------------- //
// ---- graph manipulation ------------------------------------- //
// ------------------------------------------------------------- //


#ifdef DEBUG_IMAGE_VIEW_GRAPH
void Forest::renderImage()
{
    std::stringstream dotRep;
    this->dot(dotRep);

    GVC_t* gvc = gvContext();
    Agraph_t* g = agmemread(dotRep.str().c_str());

    int old_stderr = dup(STDERR_FILENO);
    int devnull = open("/dev/null", O_WRONLY);
    dup2(devnull, STDERR_FILENO);
    close(devnull);
    gvLayout(gvc, g, "dot");
    dup2(old_stderr, STDERR_FILENO);
    close(old_stderr);

    char* data = nullptr;
    size_t length = 0;
    gvRenderData(gvc, g, "png", &data, &length);

    std::vector<uchar> pngData(data, data + length);
    cv::Mat img = cv::imdecode(pngData, cv::IMREAD_UNCHANGED);

    gvFreeRenderData(data);
    gvFreeLayout(gvc, g);
    agclose(g);
    gvFreeContext(gvc);

    this->image = img;
}
#endif

void Forest::sortChildrenAndCollectTerminals()
{
    // the number of elements in the `subtreeTerminals` vector of each node
    const unsigned int numberOfEntries = (terminalToLabel->size() + 63) / 64;

    std::function<unsigned int(Node*)> orderSubtree = [&, numberOfEntries](Node* subtreePtr) -> unsigned int {
        subtreePtr->subtreeTerminals.resize(numberOfEntries,0);
        if (terminalToLabel->contains(subtreePtr))
        {
            const unsigned int label = terminalToLabel->at(subtreePtr);
            // (label - 1) because smallest label is 1 and not 0
            subtreePtr->subtreeTerminals[(label - 1) / 64] = ((uint64_t) 1 << (label -1) % 64);
            return label;
        }
        unsigned int firstMinLabel = orderSubtree(subtreePtr->leftChild);
        unsigned int secondMinLabel = orderSubtree(subtreePtr->rightChild);
        if (firstMinLabel > secondMinLabel)
        {
            std::swap(subtreePtr->leftChild, subtreePtr->rightChild);
        }
        for(unsigned int i = 0; i < subtreePtr->subtreeTerminals.size(); i++)
        {
            subtreePtr->subtreeTerminals[i] =
                subtreePtr->leftChild->subtreeTerminals[i] |
                subtreePtr->rightChild->subtreeTerminals[i];
        }
        return std::min(firstMinLabel, secondMinLabel);
    };
    for(const auto root : *roots)
    {
        orderSubtree(root);
    }
    std::sort(roots->begin(),roots->end(),
              [&](Node* a, Node* b)
              {
                  return a->hasSmallestTerminal(b);
              });
}

std::vector<Node*> Forest::maximumCommonSubforestRoots(const Forest& other)
{
    // Find and return vector of node pointers (for this tree) corresponding to the root(s) of the maximum common sub-X-forest.
    // To do this, in both trees simultaneously iterate upward from each pair of leaves with identical label until the checked nodes' subtrees differ
    vector<Node*> newRootPtrs; // pointers of new root nodes in t1
    newRootPtrs.reserve(labelToTerminal->size());

    unordered_set<unsigned int> visitedLeaves;

    for (auto& [leafPtr, leafLabel] : *terminalToLabel)
    {
        if (visitedLeaves.contains(leafLabel)){continue;}
        Node* t1CurrentNodePtr = leafPtr;
        Node* t2CurrentNodePtr = (*other.labelToTerminal)[leafLabel];

        while (not (t1CurrentNodePtr->parent == nullptr or t2CurrentNodePtr->parent == nullptr))
        { // node is root in neither tree
            if (not hasIdenticalSubtree(t1CurrentNodePtr->sibling, t2CurrentNodePtr->sibling))
            { // sibling subtree differs
                break;
            }

            t1CurrentNodePtr = t1CurrentNodePtr->parent;
            t2CurrentNodePtr = t2CurrentNodePtr->parent;
        }
        // currentNode is now the root of the common subtree
        unsigned int label = 1;
        for (uint64_t& i : t1CurrentNodePtr->subtreeTerminals)
        {
            uint64_t temp = i;
            while (temp >= 1)
            {
                if ((temp & 1) == 1){visitedLeaves.insert(label);}
                label++;
                temp = temp >> 1;
            }
            label = label + (65 - (label % 64));
        }

        newRootPtrs.push_back(t1CurrentNodePtr);
    }

    return newRootPtrs;
}

// ------------------------------------------------------------- //
// ---- operators ---------------------------------------------- //
// ------------------------------------------------------------- //

bool Forest::operator==(const Forest& other) const
{
    std::function<bool(Node*, Node*)> compareSubtrees = [&](Node* thisNode, Node* otherNode) -> bool {

        bool thisIsTerminal = thisNode->leftChild == nullptr && thisNode->rightChild == nullptr;
        bool otherIsTerminal = otherNode->leftChild == nullptr && otherNode->rightChild == nullptr;

        if (thisIsTerminal != otherIsTerminal)
        {
            return false;
        }
        if (thisIsTerminal && otherIsTerminal)
        {
            // This check has been implemented due to the fact that the maps integrity was damaged during the refactor.
            // This may be removed, (among the other checks), if we need to scrape out every last ounce of performance
            // and it's certain again that the map will always contain the keys.
            if (not terminalToLabel->contains(thisNode) || not other.terminalToLabel->contains(otherNode))
            {
                std::cerr << "(== of Forest.cpp) terminalToLabel of either forests contains a key that isn't present within the map. "
                             "This is catastrophic failure." << "\n";
                return false;
            }

            if (terminalToLabel->contains(thisNode) and other.terminalToLabel->contains(otherNode))
            {
                return  terminalToLabel->at(thisNode) == other.terminalToLabel->at(otherNode);
            }

            return false;

        }
        return compareSubtrees(thisNode->leftChild, otherNode->leftChild) and
               compareSubtrees(thisNode->rightChild, otherNode->rightChild);
    };
    if(roots->size() != other.roots->size())
    {
        return false;
    }
    for(int i = 0; i < (int)roots->size(); ++i)
    {
        auto eq = compareSubtrees(roots->at(i), other.roots->at(i));
        if(!eq)
        {
            return false;
        }
    }
    return true;
}
// ------------------------------------------------------------- //
// ---- copy func ---------------------------------------------- //
// ------------------------------------------------------------- //


Forest Forest::copy() const
{
    // maps pointers to nodes in the og forest to pointer of corresponding nodes in the copy forest
    std::unordered_map<Node*, Node*> newPointerLookup;
    newPointerLookup.emplace(nullptr, nullptr);

    // initialize the member fields of the copy forest with correct capacity
    auto nodes_copy = std::make_shared<std::vector<Node>>();
    auto terminalToLabel_copy = std::make_shared<std::unordered_map<Node*, unsigned int>>();
    auto labelToTerminal_copy = std::make_shared<std::unordered_map<unsigned int, Node*>>();
    auto roots_copy = std::make_shared<std::vector<Node*>>();
    nodes_copy->reserve(nodes->capacity());
    terminalToLabel_copy->reserve(terminalToLabel->size());
    labelToTerminal_copy->reserve(labelToTerminal->size());
    roots_copy->reserve(roots->capacity());

    // copy the nodes with the old pointers
    for (unsigned int i = 0; i < nodes->size(); i++)
    {
        nodes_copy->push_back(nodes->at(i));
        newPointerLookup.emplace(& (*nodes)[i], & (*nodes_copy)[i]);
    }

    // adjust the pointers
    for (Node& node : *nodes_copy)
    {
        node.parent = newPointerLookup[node.parent];
        node.sibling = newPointerLookup[node.sibling];
        node.leftChild = newPointerLookup[node.leftChild];
        node.rightChild = newPointerLookup[node.rightChild];
    }

    for ( auto [terminal, label] : *terminalToLabel)
    {
        terminalToLabel_copy->emplace(newPointerLookup[terminal], label);
    }
    for (auto [label, terminal] : *labelToTerminal)
    {
        labelToTerminal_copy->emplace(label, newPointerLookup[terminal]);
    }
    for (Node* root : *roots)
    {
        roots_copy->push_back(newPointerLookup[root]);
    }

    return {nodes_copy, terminalToLabel_copy, labelToTerminal_copy, roots_copy};
}


}  //namespace graph
