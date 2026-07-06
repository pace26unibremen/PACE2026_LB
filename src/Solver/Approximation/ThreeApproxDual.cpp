#include "DualLowerBound.hpp"
#include "MutableForestView.hpp"

#include <optional>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------------------------
// 3-approximation dual (Algorithm 1 of Schalekamp, van Zuylen & van der Ster, arXiv:1511.06000).
//
// The dual objective is tracked as a single running integer `dualSum` (= sum of the paper's dual
// variables y): every finalized leaf contributes +1, every internal-node decrement contributes -1,
// and each is applied at most once per event, so the running sum equals sum(y) exactly. Then
// D = dualSum - 1 and the emitted lower bound is L = D + 1 = max(1, dualSum).
//
// This is a deliberately conservative rendering: it omits the reformulated-dual z-variables, so it is
// always valid (L <= k*) but looser than the Red-Blue 2-approx (see RedBlueDual.cpp). It runs in
// O(n^2): a single O(n) active-leaf-count sweep and an O(n) cherry scan per iteration, no hashing in
// the hot loop.
// ---------------------------------------------------------------------------------------------

namespace
{
using namespace solver::approx;

/// The two active leaves in the subtree of node index \p start (which holds exactly two). Descends only
/// into children that still carry an active leaf, so this is O(depth), not O(subtree).
std::pair<Label, Label> twoActiveLeavesUnder(const MutableForestView& view, int start)
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
std::optional<std::pair<Label, Label>> findActiveCherry(const MutableForestView& view)
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

bool isActiveSiblingPair(const MutableForestView& view, Node* aNode, Node* bNode)
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
std::optional<std::pair<Node*, Node*>> findBlockingW(const MutableForestView& view, Node* uNode, Node* vNode)
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

    approx::MutableForestView t1(*instance.at(0));
    approx::MutableForestView t2(*instance.at(1));

    approx::ActiveSet active;
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
        const approx::Label u = cherry->first;
        const approx::Label v = cherry->second;

        t2.refreshCounts(active);
        Node* uT2 = t2.node(u);
        Node* vT2 = t2.node(v);
        const int cu = t2.counts[t2.idx(t2.rootOf(uT2))];
        const int cv = t2.counts[t2.idx(t2.rootOf(vT2))];

        // line 5: u or v is alone in its T2 component -> finalize it as a singleton.
        if (cu == 1 || cv == 1)
        {
            const approx::Label w = (cu == 1) ? u : v;
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
        Node* l1 = approx::lca(t1.node(u), t1.node(v));
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
