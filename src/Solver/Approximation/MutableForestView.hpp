#ifndef PACE2026_APPROXIMATION_MUTABLE_FOREST_VIEW_HPP
#define PACE2026_APPROXIMATION_MUTABLE_FOREST_VIEW_HPP

#include "../../Graph/Forest.hpp"
#include "../../Graph/Node.hpp"

#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace solver::approx
{

using graph::Node;
using Label = unsigned int;
using ActiveSet = std::unordered_set<Label>;

constexpr int NO_NODE = -1;

inline bool isLeaf(const Node* n)
{
    // A cut turns an internal node unary (one child nulled); a genuine leaf has no children at all.
    return n->leftChild == nullptr && n->rightChild == nullptr;
}

/// A mutable clone of one input forest, indexed for fast active-leaf counting. Shared by both dual
/// lower-bound algorithms (the 3-approx and the Red-Blue 2-approx).
///
/// Every node has a fixed index (its position in \ref nodes, a pre-order listing, so a parent's index
/// is always smaller than any of its descendants'). The live tree shape is mirrored in the
/// \ref parentIdx / \ref leftIdx / \ref rightIdx arrays; \ref cut keeps both the node pointers and those
/// arrays in sync, so the count sweep and cherry scan never touch a hash map.
struct MutableForestView
{
    graph::Forest forest;  // owns the cloned nodes
    std::vector<Node*> nodes;
    std::unordered_map<const Node*, int> indexOf;
    std::unordered_map<Label, Node*> nodeOf;  // label -> leaf node
    std::vector<int> labelOf;                 // node index -> label, or 0 for internal nodes
    std::vector<int> parentIdx, leftIdx, rightIdx;
    std::vector<int> counts;  // reused active-leaf-count buffer, refilled each sweep

    explicit MutableForestView(const graph::Forest& source) : forest(source.copy())
    {
        // Collect nodes in pre-order (root before its descendants) and assign each a fixed index.
        for (Node* root : forest.Roots())
        {
            std::vector<Node*> stack{root};
            while (not stack.empty())
            {
                Node* n = stack.back();
                stack.pop_back();
                indexOf[n] = static_cast<int>(nodes.size());
                nodes.push_back(n);
                if (n->leftChild)
                    stack.push_back(n->leftChild);
                if (n->rightChild)
                    stack.push_back(n->rightChild);
            }
        }

        const int size = static_cast<int>(nodes.size());
        labelOf.assign(size, 0);
        parentIdx.assign(size, NO_NODE);
        leftIdx.assign(size, NO_NODE);
        rightIdx.assign(size, NO_NODE);
        counts.assign(size, 0);
        for (const auto& [label, node] : forest.LabelToTerminal())
        {
            nodeOf[label] = node;
            labelOf[indexOf.at(node)] = static_cast<int>(label);
        }
        for (int i = 0; i < size; ++i)
        {
            Node* n = nodes[i];
            if (n->parent)
                parentIdx[i] = indexOf.at(n->parent);
            if (n->leftChild)
                leftIdx[i] = indexOf.at(n->leftChild);
            if (n->rightChild)
                rightIdx[i] = indexOf.at(n->rightChild);
        }
    }

    int idx(const Node* n) const { return indexOf.at(n); }

    Node* node(Label label) const { return nodeOf.at(label); }

    /// Detach the edge above \p n (\p n becomes a new root), updating both the node pointers and the
    /// parent/child index arrays. No-op if \p n is already a root.
    void cut(Node* n)
    {
        Node* p = n->parent;
        if (p == nullptr)
            return;
        const int a = indexOf.at(n);
        const int pi = indexOf.at(p);
        if (p->leftChild == n)
        {
            p->leftChild = nullptr;
            leftIdx[pi] = NO_NODE;
        }
        else if (p->rightChild == n)
        {
            p->rightChild = nullptr;
            rightIdx[pi] = NO_NODE;
        }
        n->parent = nullptr;
        parentIdx[a] = NO_NODE;
    }

    /// Refill \ref counts with the number of active leaves in each node's subtree, in one O(n) sweep.
    void refreshCounts(const ActiveSet& active)
    {
        const int size = static_cast<int>(nodes.size());
        for (int i = 0; i < size; ++i)
        {
            const int label = labelOf[i];
            counts[i] = (label != 0 && active.contains(static_cast<Label>(label))) ? 1 : 0;
        }
        // nodes is pre-order, so descending i visits every child before its parent.
        for (int i = size - 1; i >= 0; --i)
            if (parentIdx[i] != NO_NODE)
                counts[parentIdx[i]] += counts[i];
    }

    int countAt(int i) const { return i == NO_NODE ? 0 : counts[i]; }

    Node* rootOf(Node* n) const
    {
        while (n->parent != nullptr)
            n = n->parent;
        return n;
    }

    // ----------------------------------------------------------------------------------------------
    // Static-tree LCA acceleration. ONLY valid while the forest is not cut (i.e. the original/topology
    // clones). Binary lifting: O(n log n) build, O(log n) query. Used to make the triplet-consistency
    // check (outlier) cheap -- it is the dominant cost on large instances, and the original trees never
    // change during the pass, so their depths and ancestors are fixed.
    // ----------------------------------------------------------------------------------------------
    std::vector<int> depth;             // node index -> depth (root = 0); empty until buildStaticLca()
    std::vector<std::vector<int>> anc;  // anc[k][i] = 2^k-th ancestor index of i, or NO_NODE
    int logN = 1;

    void buildStaticLca()
    {
        const int size = static_cast<int>(nodes.size());
        depth.assign(size, 0);
        // nodes is pre-order, so parentIdx[i] < i: parents are finalised before their children.
        for (int i = 0; i < size; ++i)
            depth[i] = (parentIdx[i] == NO_NODE) ? 0 : depth[parentIdx[i]] + 1;
        logN = 1;
        while ((1 << logN) < (size > 0 ? size : 1))
            ++logN;
        anc.assign(logN, std::vector<int>(size, NO_NODE));
        for (int i = 0; i < size; ++i)
            anc[0][i] = parentIdx[i];
        for (int k = 1; k < logN; ++k)
            for (int i = 0; i < size; ++i)
                anc[k][i] = (anc[k - 1][i] == NO_NODE) ? NO_NODE : anc[k - 1][anc[k - 1][i]];
    }

    /// LCA via the precomputed table (buildStaticLca must have run). O(log n). nullptr if disconnected.
    Node* lcaFast(Node* aNode, Node* bNode) const
    {
        int a = indexOf.at(aNode);
        int b = indexOf.at(bNode);
        if (depth[a] < depth[b])
            std::swap(a, b);
        int diff = depth[a] - depth[b];
        for (int k = 0; k < logN; ++k)
            if (diff & (1 << k))
            {
                a = anc[k][a];
                if (a == NO_NODE)
                    return nullptr;
            }
        if (a == b)
            return nodes[a];
        for (int k = logN - 1; k >= 0; --k)
            if (anc[k][a] != anc[k][b])
            {
                a = anc[k][a];
                b = anc[k][b];
            }
        return anc[0][a] == NO_NODE ? nullptr : nodes[anc[0][a]];
    }
};

/// LCA of \p a and \p b within the current forest, or nullptr if they are in different components.
/// Allocation-free: equalize depths by parent-walking, then ascend in lockstep until the pointers meet
/// (or both reach a null root, i.e. different components). O(depth), no heap traffic in the hot loop.
inline Node* lca(Node* a, Node* b)
{
    int da = 0;
    for (Node* x = a; x != nullptr; x = x->parent)
        ++da;
    int db = 0;
    for (Node* x = b; x != nullptr; x = x->parent)
        ++db;
    while (da > db)
    {
        a = a->parent;
        --da;
    }
    while (db > da)
    {
        b = b->parent;
        --db;
    }
    while (a != b)  // meets at the LCA, or both become nullptr in different components
    {
        a = a->parent;
        b = b->parent;
    }
    return a;
}

}  // namespace solver::approx

#endif  // PACE2026_APPROXIMATION_MUTABLE_FOREST_VIEW_HPP
