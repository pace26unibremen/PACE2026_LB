#include "DualLowerBound.hpp"

#include "../Graph/Forest.hpp"
#include "../Graph/Node.hpp"

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------------------------
// The dual objective is tracked as a single running integer `dualSum` (= sum of the paper's dual
// variables y): every finalized leaf contributes +1, every internal-node decrement contributes -1,
// and each is applied at most once per event, so the running sum equals sum(y) exactly. Then
// D = dualSum - 1 and the emitted lower bound is L = D + 1 = max(1, dualSum).
//
// The construction is deliberately conservative rather than the paper's fully faithful dual: it omits
// the reformulated-dual z-variables. A faithful version would raise more y-credit per conflict and so
// give a tighter (larger) L, but keeping it valid (L <= k*) under that extra credit is delicate --
// an over-credit anywhere makes L > k* and would certify an invalid answer. This rendering only ever
// credits +1 per finalized leaf and debits at conflict LCAs / cut pendants, which stays valid.
//
// Performance: every node gets a fixed integer index up front, and the current tree topology is held in
// parent/left/right index arrays (updated in O(1) on each cut). Per iteration the active-leaf count of
// every node is recomputed in a single O(n) array sweep (no hashing in the hot loop), and cherry finding
// is an O(n) array scan. That keeps the whole pass O(n^2) -- the paper's bound -- with small constants.
// ---------------------------------------------------------------------------------------------

namespace
{
using graph::Node;
using Label = unsigned int;
using ActiveSet = std::unordered_set<Label>;

constexpr int NO_NODE = -1;

bool isLeaf(const Node* n)
{
    // A cut turns an internal node unary (one child nulled); a genuine leaf has no children at all.
    return n->leftChild == nullptr && n->rightChild == nullptr;
}

/// A mutable clone of one input forest, indexed for fast active-leaf counting.
///
/// Every node has a fixed index (its position in `nodes`, which is a pre-order listing, so a parent's
/// index is always smaller than any of its descendants'). The live tree shape is mirrored in the
/// `parentIdx`/`leftIdx`/`rightIdx` arrays; \ref cut keeps both the node pointers and those arrays in
/// sync, so the count sweep and cherry scan never touch a hash map.
struct ForestView
{
    graph::Forest forest;  // owns the cloned nodes
    std::vector<Node*> nodes;
    std::unordered_map<const Node*, int> indexOf;
    std::unordered_map<Label, Node*> nodeOf;  // label -> leaf node
    std::vector<int> labelOf;                 // node index -> label, or 0 for internal nodes
    std::vector<int> parentIdx, leftIdx, rightIdx;
    std::vector<int> counts;  // reused active-leaf-count buffer, refilled each sweep

    explicit ForestView(const graph::Forest& source) : forest(source.copy())
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
};

/// LCA of \p a and \p b within the current forest, or nullptr if they are in different components.
Node* lca(Node* a, Node* b)
{
    std::unordered_set<const Node*> anc;
    for (Node* x = a; x != nullptr; x = x->parent)
        anc.insert(x);
    for (Node* x = b; x != nullptr; x = x->parent)
        if (anc.contains(x))
            return x;
    return nullptr;
}

/// The two active leaves in the subtree of node index \p start (which holds exactly two). Descends only
/// into children that still carry an active leaf, so this is O(depth), not O(subtree).
std::pair<Label, Label> twoActiveLeavesUnder(const ForestView& view, int start)
{
    std::vector<Label> found;
    std::vector<int> stack{start};
    while (not stack.empty() && found.size() < 2)
    {
        const int i = stack.back();
        stack.pop_back();
        if (view.labelOf[i] != 0)
        {
            found.push_back(static_cast<Label>(view.labelOf[i]));
        }
        else
        {
            if (view.leftIdx[i] != NO_NODE && view.counts[view.leftIdx[i]] > 0)
                stack.push_back(view.leftIdx[i]);
            if (view.rightIdx[i] != NO_NODE && view.counts[view.rightIdx[i]] > 0)
                stack.push_back(view.rightIdx[i]);
        }
    }
    return {found[0], found[1]};
}

/// The lowest internal node whose subtree holds exactly two active leaves (an active sibling pair),
/// read from the precomputed counts. Returns the two leaf labels, or nullopt if none exists. Scans in
/// pre-order and requires both children to hold < 2, so it returns the pre-order-first lowest such node
/// (identical selection to the reference construction).
std::optional<std::pair<Label, Label>> findActiveCherry(const ForestView& view)
{
    const int size = static_cast<int>(view.nodes.size());
    for (int i = 0; i < size; ++i)
    {
        if (view.labelOf[i] != 0 || view.counts[i] != 2)
            continue;
        if (view.countAt(view.leftIdx[i]) >= 2 || view.countAt(view.rightIdx[i]) >= 2)
            continue;
        return twoActiveLeavesUnder(view, i);
    }
    return std::nullopt;
}

bool isActiveSiblingPair(const ForestView& view, Node* aNode, Node* bNode)
{
    if (view.rootOf(aNode) != view.rootOf(bNode))
        return false;
    Node* m = lca(aNode, bNode);
    return m != nullptr && view.counts[view.idx(m)] == 2;
}

/// If u,v sit in the same component separated by an active pendant, return (W, p2W): the first off-path
/// subtree carrying an active leaf on the u-v path and its attachment (path) node. This is the paper's
/// p2(W) / Whidden's b-node, walking up from u and from v toward (but not including) their LCA. Returns
/// nullopt when u,v are already adjacent (no active pendant between them).
std::optional<std::pair<Node*, Node*>> findBlockingW(const ForestView& view, Node* uNode, Node* vNode)
{
    Node* m = lca(uNode, vNode);
    if (m == nullptr)
        return std::nullopt;
    for (Node* leaf : {uNode, vNode})
    {
        Node* child = leaf;
        Node* node = leaf->parent;
        while (node != nullptr && node != m)
        {
            for (Node* ch : {node->leftChild, node->rightChild})
            {
                if (ch == nullptr || ch == child)
                    continue;
                if (view.counts[view.idx(ch)] > 0)
                    return std::make_pair(ch, node);
            }
            child = node;
            node = node->parent;
        }
    }
    return std::nullopt;
}
}  // namespace

long solver::computeDual3ApproxLowerBound(const graph::Instance& instance)
{
    // The dual construction of Algorithm 1 is defined for two rooted binary trees. For any other shape
    // fall back to the universally-valid trivial bound: every agreement forest has >= 1 component, so
    // L = 1 <= k*.
    if (instance.size() != 2)
        return 1;

    ForestView t1(*instance.at(0));
    ForestView t2(*instance.at(1));

    ActiveSet active;
    active.reserve(t1.nodeOf.size());
    for (const auto& [label, node] : t1.nodeOf)
        active.insert(label);

    long dualSum = 0;

    while (active.size() >= 2)
    {
        t1.refreshCounts(active);
        auto cherry = findActiveCherry(t1);
        if (not cherry.has_value())
            break;
        const Label u = cherry->first;
        const Label v = cherry->second;

        t2.refreshCounts(active);
        Node* uT2 = t2.node(u);
        Node* vT2 = t2.node(v);
        const int cu = t2.counts[t2.idx(t2.rootOf(uT2))];
        const int cv = t2.counts[t2.idx(t2.rootOf(vT2))];

        // line 5: u or v is alone in its T2 component -> finalize it as a singleton.
        if (cu == 1 || cv == 1)
        {
            const Label w = (cu == 1) ? u : v;
            t1.cut(t1.node(w));
            t2.cut(t2.node(w));
            active.erase(w);
            dualSum += 1;  // y_w <- 1
            continue;
        }

        // line 8: u,v are also an active sibling pair in T2 (a common cherry) -> merge, no dual change.
        if (isActiveSiblingPair(t2, uT2, vT2))
        {
            active.erase(u);  // u becomes inactive, represented by v
            continue;
        }

        // lines 10-13: conflict. Optionally cut a blocking active pendant W in T2 first.
        if (t2.rootOf(uT2) == t2.rootOf(vT2))
        {
            if (auto blocking = findBlockingW(t2, uT2, vT2))
            {
                t2.cut(blocking->first);
                dualSum -= 1;  // y_{p2(W)} <- y_{p2(W)} - 1
            }
        }

        // line 13: cut u and v off in both forests, finalize both, decrement y at lca1(u,v).
        Node* l1 = lca(t1.node(u), t1.node(v));
        t1.cut(t1.node(u));
        t1.cut(t1.node(v));
        t2.cut(uT2);
        t2.cut(vT2);
        active.erase(u);
        active.erase(v);
        dualSum += 2;  // y_u <- 1, y_v <- 1
        if (l1 != nullptr)
            dualSum -= 1;  // y_{lca1(u,v)} <- y_{lca1(u,v)} - 1
    }

    // line 18: finalize the last remaining active leaf.
    if (active.size() == 1)
        dualSum += 1;

    // D = dualSum - 1, L = D + 1 = dualSum, clamped to the trivial floor of 1 (>= 1 component).
    return dualSum > 1 ? dualSum : 1;
}
