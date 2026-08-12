#include "DualLowerBound.hpp"
#include "MutableForestView.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace solver::approx;

// =================================================================================================
// Red-Blue 2-approximation dual (Algorithm 2 + Procedures 1-4c). See DualLowerBound.hpp and, for the
// per-case boxed y-updates, proto/ALG2_SPEC.md in the lp-duality worktree. We track only dualSum = sum(y);
// the bound is L = dualSum. Retroactive merges (Proc 4b/4c) never touch y, so they are skipped.
//
// Two clones of each input forest are kept: a WORKING copy (mutated by cuts) for structural predicates
// (same tree / active siblings / alone-in-tree) and an ORIGINAL copy (never cut) for the topology
// relation "uv|w" and triplet consistency. Predicates walk subtrees directly against the active set
// rather than the counts array, so no count bookkeeping can go stale across the many mutation kinds.
// =================================================================================================
namespace
{
int labelOfNode(const MutableForestView& v, const Node* n) { return v.labelOf[v.indexOf.at(n)]; }

/// Active leaf labels in the subtree rooted at \p n of working/original view \p v.
std::vector<Label> activeLabelsUnder(const MutableForestView& v, Node* n, const ActiveSet& active)
{
    std::vector<Label> out;
    std::vector<Node*> stack{n};
    while (not stack.empty())
    {
        Node* x = stack.back();
        stack.pop_back();
        const int lbl = labelOfNode(v, x);
        if (lbl != 0)
        {
            if (active.contains(static_cast<Label>(lbl)))
                out.push_back(static_cast<Label>(lbl));
        }
        else
        {
            if (x->leftChild)
                stack.push_back(x->leftChild);
            if (x->rightChild)
                stack.push_back(x->rightChild);
        }
    }
    return out;
}

/// The Red-Blue dual construction as a self-contained pass over four ForestViews.
struct RedBlue2
{
    MutableForestView &t1, &t2;    // working (mutated)
    MutableForestView &t1o, &t2o;  // original (topology)
    ActiveSet& active;
    long dualSum = 0;
    bool sawStar = false;  // FinalCut or Merge-After-Cut seen in the current ResolveSet
    solver::Deadline deadline = solver::noDeadline();
    const std::atomic<bool>* stop = nullptr;  // cooperative-cancellation flag (or null)
    bool aborted = false;  // set if the deadline was hit or cancellation requested before finishing

    RedBlue2(MutableForestView& w1, MutableForestView& w2, MutableForestView& o1, MutableForestView& o2,
             ActiveSet& a, solver::Deadline d, const std::atomic<bool>* s)
        : t1(w1), t2(w2), t1o(o1), t2o(o2), active(a), deadline(d), stop(s)
    {
        // The original (topology) clones are never cut, so precompute their LCA tables once: the
        // triplet-consistency check (outlier) is the dominant cost at scale and queries them heavily.
        t1o.buildStaticLca();
        t2o.buildStaticLca();
    }

    bool timedOut() const
    {
        // Written as `deadline < now` (not `now > deadline`): when UWrMaxSat is in the link (this
        // branch), its Global.h leaks an operator that makes the `>` form ambiguous on time_points.
        return (stop != nullptr && stop->load(std::memory_order_relaxed))
            || deadline < std::chrono::steady_clock::now();
    }

    // ---- active-leaf counts (lazy) ----
    // counts[node] in each WORKING view = active leaves under node in the current forest. Refreshed
    // lazily: every mutation (deactivate / cutEdge) marks them dirty; the next count query rebuilds both
    // views in one O(n) sweep, then serves O(1) reads until the next mutation. This turns the structural
    // predicates from per-call O(subtree) walks (the dominant cost -- see profiling) into O(1) lookups.
    bool countsDirty = true;

    void ensureCounts()
    {
        if (countsDirty)
        {
            t1.refreshCounts(active);
            t2.refreshCounts(active);
            countsDirty = false;
        }
    }

    void deactivate(Label u)
    {
        active.erase(u);
        countsDirty = true;
    }

    void cutEdge(MutableForestView& v, Node* n)
    {
        v.cut(n);
        countsDirty = true;
    }

    /// Active leaves under \p n in working view \p v (current forest); O(1) after an amortized refresh.
    int countUnder(MutableForestView& v, Node* n)
    {
        ensureCounts();
        return v.counts[v.idx(n)];
    }

    bool hasActiveUnder(MutableForestView& v, Node* n) { return countUnder(v, n) > 0; }

    // ---- topology on ORIGINAL trees ----
    /// The "outlier" of triplet {a,b,c} in original view \p v: the leaf split off from the closer pair
    /// (xy|z => outlier z). Returns 0 if degenerate. Uses the deepest of the three pairwise LCAs.
    Label outlier(MutableForestView& v, Label a, Label b, Label c)
    {
        // v is always an original (topology) view, so its static LCA table is built: O(log n) per query.
        Node* na = v.node(a);
        Node* nb = v.node(b);
        Node* nc = v.node(c);
        Node* lab = v.lcaFast(na, nb);
        Node* lac = v.lcaFast(na, nc);
        Node* lbc = v.lcaFast(nb, nc);
        const int dab = lab ? v.depth[v.idx(lab)] : -1;
        const int dac = lac ? v.depth[v.idx(lac)] : -1;
        const int dbc = lbc ? v.depth[v.idx(lbc)] : -1;
        if (dab >= dac && dab >= dbc)
            return (dab > dac || dab > dbc) ? c : 0;  // a,b closest -> outlier c
        if (dac >= dab && dac >= dbc)
            return (dac > dab || dac > dbc) ? b : 0;
        return (dbc > dab || dbc > dac) ? a : 0;
    }

    bool consistent(Label a, Label b, Label c) { return outlier(t1o, a, b, c) == outlier(t2o, a, b, c); }

    /// uv|w in ORIGINAL tree \p v: u,v are the closer pair (outlier is w).
    bool restrictsO(MutableForestView& v, Label u, Label vv, Label w) { return outlier(v, u, vv, w) == w; }

    // ---- structural predicates on WORKING forests ----
    bool sameTree(MutableForestView& v, Label a, Label b) { return v.rootOf(v.node(a)) == v.rootOf(v.node(b)); }

    bool aloneInTree(MutableForestView& v, Label u) { return countUnder(v, v.rootOf(v.node(u))) == 1; }

    bool activeSiblings(MutableForestView& v, Label a, Label b)
    {
        if (not sameTree(v, a, b))
            return false;
        Node* m = lca(v.node(a), v.node(b));
        return m != nullptr && countUnder(v, m) == 2;
    }

    /// Reference O(|s|^3) compatibility: no inconsistent triplet. Kept for the env-gated cross-check.
    bool compatibleSlow(const std::vector<Label>& s)
    {
        for (std::size_t i = 0; i < s.size(); ++i)
            for (std::size_t j = i + 1; j < s.size(); ++j)
                for (std::size_t k = j + 1; k < s.size(); ++k)
                    if (not consistent(s[i], s[j], s[k]))
                        return false;
        return true;
    }

    /// The leaf-position ranges [lo,hi] of the clusters of the tree induced on \p order by view \p v.
    /// \p order must be sorted by v's pre-order. The induced tree is the min-Cartesian tree over the
    /// adjacent-pair LCA depths (for a binary tree the min in every subrange is unique, so this is
    /// unambiguous); each internal node owns a contiguous leaf interval = a cluster.
    std::vector<std::pair<int, int>> clusterRanges(MutableForestView& v, const std::vector<Label>& order)
    {
        const int m = static_cast<int>(order.size());
        std::vector<int> a(m - 1);
        for (int i = 0; i < m - 1; ++i)
            a[i] = v.depth[v.idx(v.lcaFast(v.node(order[i]), v.node(order[i + 1])))];
        std::vector<int> prevS(m - 1), nextS(m - 1), st;
        for (int i = 0; i < m - 1; ++i)  // previous strictly-smaller
        {
            while (not st.empty() && a[st.back()] >= a[i])
                st.pop_back();
            prevS[i] = st.empty() ? -1 : st.back();
            st.push_back(i);
        }
        st.clear();
        for (int i = m - 2; i >= 0; --i)  // next smaller-or-equal
        {
            while (not st.empty() && a[st.back()] > a[i])
                st.pop_back();
            nextS[i] = st.empty() ? (m - 1) : st.back();
            st.push_back(i);
        }
        std::vector<std::pair<int, int>> ranges(m - 1);
        for (int i = 0; i < m - 1; ++i)
            ranges[i] = {prevS[i] + 1, nextS[i]};  // leaf positions [lo,hi]
        return ranges;
    }

    /// O(|s| log |s|) compatibility: the trees induced on s by T1 and T2 are isomorphic iff they have
    /// the same clusters. Number s by T1's leaf order; every T1 cluster is then a contiguous interval.
    /// For each T2 cluster, check its T1-numbers form the same interval and that interval is a T1
    /// cluster. Both induced trees are binary with |s|-1 clusters, so all-T2-in-T1 implies equality.
    bool compatibleFast(const std::vector<Label>& s)
    {
        const int k = static_cast<int>(s.size());
        if (k <= 2)
            return true;
        std::vector<Label> s1(s), s2(s);
        std::sort(s1.begin(), s1.end(),
                  [&](Label x, Label y) { return t1o.idx(t1o.node(x)) < t1o.idx(t1o.node(y)); });
        std::sort(s2.begin(), s2.end(),
                  [&](Label x, Label y) { return t2o.idx(t2o.node(x)) < t2o.idx(t2o.node(y)); });
        std::unordered_map<Label, int> rank1;
        rank1.reserve(k * 2);
        for (int i = 0; i < k; ++i)
            rank1[s1[i]] = i;

        auto key = [](int lo, int hi) { return (static_cast<long long>(lo) << 24) | hi; };
        std::unordered_set<long long> t1clusters;
        t1clusters.reserve(k * 2);
        for (auto [lo, hi] : clusterRanges(t1o, s1))
            t1clusters.insert(key(lo, hi));

        // sparse tables for range min/max of the T1-rank sequence in T2 order
        std::vector<int> r2(k);
        for (int i = 0; i < k; ++i)
            r2[i] = rank1[s2[i]];
        int logn = 1;
        while ((1 << logn) <= k)  // need floor(log2(k)) + 1 levels (rmin/rmax indexes row floor(log2(len)))
            ++logn;
        std::vector<std::vector<int>> mn(logn, std::vector<int>(k)), mx(logn, std::vector<int>(k));
        for (int i = 0; i < k; ++i)
            mn[0][i] = mx[0][i] = r2[i];
        for (int j = 1; j < logn; ++j)
            for (int i = 0; i + (1 << j) <= k; ++i)
            {
                mn[j][i] = std::min(mn[j - 1][i], mn[j - 1][i + (1 << (j - 1))]);
                mx[j][i] = std::max(mx[j - 1][i], mx[j - 1][i + (1 << (j - 1))]);
            }
        auto rmin = [&](int l, int r) {
            const int j = 31 - __builtin_clz(static_cast<unsigned>(r - l + 1));
            return std::min(mn[j][l], mn[j][r - (1 << j) + 1]);
        };
        auto rmax = [&](int l, int r) {
            const int j = 31 - __builtin_clz(static_cast<unsigned>(r - l + 1));
            return std::max(mx[j][l], mx[j][r - (1 << j) + 1]);
        };

        for (auto [x, y] : clusterRanges(t2o, s2))
        {
            const int lo = rmin(x, y), hi = rmax(x, y);
            if (hi - lo + 1 != y - x + 1)          // T2 cluster not contiguous in T1's numbering
                return false;
            if (not t1clusters.contains(key(lo, hi)))  // ... or not a T1 cluster
                return false;
        }
        return true;
    }

    bool compatibleSet(const std::vector<Label>& s)
    {
        // Small sets: the O(k^3) check early-exits and has no sort/sparse-table overhead, so it wins.
        // Large sets: the O(k log k) cluster comparison wins (and, for compatible sets, the cubic path
        // has no early exit). Both give the identical answer (verified by the RB_CHECK_COMPAT gate).
        if (std::getenv("RB_CHECK_COMPAT") == nullptr)
            return s.size() <= 64 ? compatibleSlow(s) : compatibleFast(s);
        const bool fast = compatibleFast(s);
        // Opt-in cross-check against the O(k^3) reference on real workloads before the slow path is
        // retired: RB_CHECK_COMPAT=1 aborts on any disagreement.
        if (std::getenv("RB_CHECK_COMPAT") != nullptr && fast != compatibleSlow(s))
        {
            std::cerr << "RB_CHECK_COMPAT mismatch: fast=" << fast << " slow=" << compatibleSlow(s)
                      << " |s|=" << s.size() << "\n";
            std::abort();
        }
        return fast;
    }

    // ---- cut primitives on WORKING forests ----
    /// Cut the edge below p(u) toward u (detaches the subtree whose only active leaf is u). No-op if u is
    /// the only active leaf in its tree.
    void cutOffLeaf(MutableForestView& v, Label u)
    {
        Node* child = v.node(u);
        while (child->parent != nullptr)
        {
            Node* parent = child->parent;
            for (Node* sib : {parent->leftChild, parent->rightChild})
            {
                if (sib == nullptr || sib == child)
                    continue;
                if (hasActiveUnder(v, sib))
                {
                    cutEdge(v, child);
                    return;
                }
            }
            child = parent;
        }
    }

    /// ResolvePair line 13: cut the active pendant hanging below p(u) that is NOT toward u.
    void cutPendant(MutableForestView& v, Label u)
    {
        Node* child = v.node(u);
        while (child->parent != nullptr)
        {
            Node* parent = child->parent;
            for (Node* sib : {parent->leftChild, parent->rightChild})
            {
                if (sib == nullptr || sib == child)
                    continue;
                if (hasActiveUnder(v, sib))
                {
                    cutEdge(v, sib);
                    return;
                }
            }
            child = parent;
        }
    }

    /// True if p(u) is strictly below lca(u,v) in \p v (an active pendant strictly between u and lca).
    bool pBelowLca(MutableForestView& v, Label u, Label vv)
    {
        Node* m = lca(v.node(u), v.node(vv));
        Node* child = v.node(u);
        while (child->parent != nullptr && child->parent != m)
        {
            Node* parent = child->parent;
            for (Node* sib : {parent->leftChild, parent->rightChild})
            {
                if (sib == nullptr || sib == child)
                    continue;
                if (hasActiveUnder(v, sib))
                    return true;
            }
            child = parent;
        }
        return false;
    }

    /// Paper line 2 relabel test: does the ORIGINAL lca2(u,v) node currently sit in u's working T2 tree?
    /// The working (t2) and original (t2o) views are independent clones of the same forest built by the
    /// identical pre-order, so node index i denotes the same original node in both.
    bool origLca2InTreeOf(Label u, Label v)
    {
        Node* lo = lca(t2o.node(u), t2o.node(v));
        if (lo == nullptr)
            return false;
        Node* loWork = t2.nodes[t2o.indexOf.at(lo)];
        return t2.rootOf(loWork) == t2.rootOf(t2.node(u));
    }

    /// Does u's working T2 tree hold an active leaf other than u that is NOT in U? (FinalCut condition.)
    bool uTreeHasActiveOutsideU(Label u, const std::unordered_set<Label>& U)
    {
        for (Label w : activeLabelsUnder(t2, t2.rootOf(t2.node(u)), active))
            if (w != u && not U.contains(w))
                return true;
        return false;
    }

    // ---- ResolvePair (Procedure 1) ----
    void resolvePair(Label u, Label vv, const std::unordered_set<Label>& U)
    {
        if (not sameTree(t2, u, vv))
        {
            // line 2: relabel so lca2(u,v) is NOT in the tree containing u.
            if (origLca2InTreeOf(u, vv))
                std::swap(u, vv);
            if (uTreeHasActiveOutsideU(u, U))
            {
                cutOffLeaf(t2, u);
                cutOffLeaf(t1, u);
                deactivate(u);
                dualSum += 1;  // FinalCut, y_u <- 1
                sawStar = true;
            }
            else
            {
                cutOffLeaf(t1, u);  // line 6: cut in T1' only
                deactivate(u);
                dualSum += 1;  // y_u <- 1
            }
            return;
        }
        if (activeSiblings(t2, u, vv))
        {
            deactivate(u);  // merge, no dual change
            return;
        }
        if (not pBelowLca(t2, u, vv))
            std::swap(u, vv);
        cutPendant(t2, u);
        dualSum -= 1;  // y_{p2(u)} <- y_{p2(u)} - 1
        if (activeSiblings(t2, u, vv))
        {
            deactivate(u);  // Merge-After-Cut
            dualSum += 1;     // y_u <- 1
            sawStar = true;
        }
        else
        {
            cutOffLeaf(t1, u);
            cutOffLeaf(t2, u);
            deactivate(u);
            dualSum += 1;  // y_u <- 1
        }
    }

    std::vector<Label> activeIn(const std::vector<Label>& s)
    {
        std::vector<Label> out;
        for (Label x : s)
            if (active.contains(x))
                out.push_back(x);
        return out;
    }

    /// The two active leaves under node index \p start of view \p v (which holds exactly two), read via
    /// the counts array -- descends only into children that still carry an active leaf, so O(depth).
    std::pair<Label, Label> twoActiveUnder(MutableForestView& v, int start)
    {
        Label found[2] = {0, 0};
        int nf = 0;
        std::vector<int> stack{start};
        while (not stack.empty() && nf < 2)
        {
            const int i = stack.back();
            stack.pop_back();
            if (v.labelOf[i] != 0)
                found[nf++] = static_cast<Label>(v.labelOf[i]);
            else
            {
                if (v.leftIdx[i] != NO_NODE && v.counts[v.leftIdx[i]] > 0)
                    stack.push_back(v.leftIdx[i]);
                if (v.rightIdx[i] != NO_NODE && v.counts[v.rightIdx[i]] > 0)
                    stack.push_back(v.rightIdx[i]);
            }
        }
        return {found[0], found[1]};
    }

    /// The single active leaf under node index \p start of view \p v (which holds exactly one).
    Label oneActiveUnder(MutableForestView& v, int start)
    {
        int i = start;
        while (v.labelOf[i] == 0)
            i = (v.leftIdx[i] != NO_NODE && v.counts[v.leftIdx[i]] > 0) ? v.leftIdx[i] : v.rightIdx[i];
        return static_cast<Label>(v.labelOf[i]);
    }

    /// A T1 active cherry (lowest node holding exactly two active leaves) that is also an active sibling
    /// pair in T2, or {0,0}. O(n) counts scan -- replaces preprocess's old O(k^2) pairwise search.
    std::pair<Label, Label> findBothForestCherry()
    {
        ensureCounts();
        const int size = static_cast<int>(t1.nodes.size());
        for (int i = 0; i < size; ++i)
        {
            if (t1.labelOf[i] != 0 || t1.counts[i] != 2)
                continue;
            if (t1.countAt(t1.leftIdx[i]) >= 2 || t1.countAt(t1.rightIdx[i]) >= 2)
                continue;
            auto [u, v] = twoActiveUnder(t1, i);
            if (activeSiblings(t2, u, v))
                return {u, v};
        }
        return {0, 0};
    }

    /// An active leaf that is the only active leaf in its T2 tree (and not the last active leaf overall),
    /// or 0. O(n) scan of the T2 component roots.
    Label findT2Alone()
    {
        if (active.size() <= 1)
            return 0;
        ensureCounts();
        const int size = static_cast<int>(t2.nodes.size());
        for (int i = 0; i < size; ++i)
            if (t2.parentIdx[i] == NO_NODE && t2.counts[i] == 1)
                return oneActiveUnder(t2, i);
        return 0;
    }

    /// An active sibling pair in T1' with both leaves in \p s, or {0,0} if none. O(n) counts scan (s is
    /// an active sibling set, so its leaves are exactly the active leaves under lca1(s); a T1 cherry with
    /// both leaves in s is therefore a sibling pair within s).
    std::pair<Label, Label> activeSiblingPairIn(const std::vector<Label>& s)
    {
        ensureCounts();
        const std::unordered_set<Label> sset(s.begin(), s.end());
        const int size = static_cast<int>(t1.nodes.size());
        for (int i = 0; i < size; ++i)
        {
            if (t1.labelOf[i] != 0 || t1.counts[i] != 2)
                continue;
            if (t1.countAt(t1.leftIdx[i]) >= 2 || t1.countAt(t1.rightIdx[i]) >= 2)
                continue;
            auto [u, v] = twoActiveUnder(t1, i);
            if (sset.contains(u) && sset.contains(v))
                return {u, v};
        }
        return {0, 0};
    }

    // ---- ResolveSet (Procedure 2) ----
    bool resolveSet(const std::vector<Label>& R)  // returns true = Success
    {
        const std::unordered_set<Label> Uset(R.begin(), R.end());
        sawStar = false;
        dualSum -= 1;  // line 21: y_{lca1(R)} <- y_{lca1(R)} - 1
        while (static_cast<int>(activeIn(R).size()) >= 3)
        {
            auto [a, b] = activeSiblingPairIn(R);
            if (a == 0)
                break;
            resolvePair(a, b, Uset);
        }
        std::vector<Label> rem = activeIn(R);
        if (rem.size() < 2)
            return true;  // collapsed early (e.g. FinalCuts): progress made
        Label uh = rem[0], vh = rem[1];
        if (activeSiblings(t2, uh, vh))
        {
            deactivate(uh);
            dualSum += 1;  // merge, y_uh <- 1
            return true;
        }
        bool condDiff = not sameTree(t2, uh, vh);
        bool hasW = false;
        if (not condDiff)
        {
            for (Label w : activeLabelsUnder(t2, t2.rootOf(t2.node(uh)), active))
            {
                if (w == uh || w == vh)
                    continue;
                if (restrictsO(t2o, uh, vh, w))
                {
                    hasW = true;
                    break;
                }
            }
        }
        if (condDiff || not hasW)
        {
            if (not aloneInTree(t2, uh))
                cutOffLeaf(t2, uh);
            cutOffLeaf(t1, uh);
            deactivate(uh);
            dualSum += 1;  // y_uh <- 1
            if (not aloneInTree(t2, vh))
                cutOffLeaf(t2, vh);
            cutOffLeaf(t1, vh);
            deactivate(vh);
            dualSum += 1;  // y_vh <- 1
            return true;
        }
        if (sawStar)
        {
            resolvePair(uh, vh, Uset);
            std::vector<Label> last = activeIn(R);
            if (not last.empty())
            {
                Label lv = last[0];
                if (not aloneInTree(t2, lv))
                    cutOffLeaf(t2, lv);
                cutOffLeaf(t1, lv);
                deactivate(lv);
                dualSum += 1;  // y_vh <- 1
            }
            return true;
        }
        return false;  // Fail
    }

    // ---- minimal incompatible active sibling set ----
    Node* lcaSet(MutableForestView& v, const std::vector<Label>& labels)
    {
        if (labels.empty())
            return nullptr;
        Node* cur = v.node(labels[0]);
        for (std::size_t i = 1; i < labels.size() && cur != nullptr; ++i)
            cur = lca(cur, v.node(labels[i]));
        return cur;
    }

    /// Find (R, B) with R u B a minimal incompatible active sibling set; R has >=2 leaves. Returns false
    /// if none (all remaining active leaves are mutually compatible).
    bool findMinIncompat(std::vector<Label>& R, std::vector<Label>& B)
    {
        std::vector<Label> act(active.begin(), active.end());
        if (act.size() < 2)
            return false;
        Node* node = lcaSet(t1, act);
        while (node != nullptr)
        {
            std::vector<std::vector<Label>> childsets;
            for (Node* ch : {node->leftChild, node->rightChild})
                if (ch != nullptr)
                {
                    auto s = activeLabelsUnder(t1, ch, active);
                    if (not s.empty())
                        childsets.push_back(std::move(s));
                }
            if (childsets.size() < 2)
            {
                Node* nxt = nullptr;
                for (Node* ch : {node->leftChild, node->rightChild})
                    if (ch != nullptr)
                    {
                        auto s = activeLabelsUnder(t1, ch, active);
                        if (not s.empty())
                        {
                            nxt = lcaSet(t1, s);
                            break;
                        }
                    }
                if (nxt == nullptr || nxt == node)
                    return false;
                node = nxt;
                continue;
            }
            std::vector<Label>& A = childsets[0];
            std::vector<Label>& C = childsets[1];
            if (compatibleSet(A) && compatibleSet(C))
            {
                orient(A, C, R, B);
                return true;
            }
            node = lcaSet(t1, compatibleSet(A) ? C : A);
        }
        return false;
    }

    /// Orient so R has >=2 leaves (ResolveSet distills it to the pair r1,r2), preferring the
    /// lca2-spanning side (which is never a singleton).
    void orient(std::vector<Label>& A, std::vector<Label>& C, std::vector<Label>& R, std::vector<Label>& B)
    {
        std::vector<Label> uni;
        uni.insert(uni.end(), A.begin(), A.end());
        uni.insert(uni.end(), C.begin(), C.end());
        Node* mu = lcaSet(t2, uni);
        auto pick = [&](std::vector<Label>& X, std::vector<Label>& Y) {
            R = X;
            B = Y;
        };
        const bool aBig = A.size() >= 2, cBig = C.size() >= 2;
        if (mu != nullptr)
        {
            if (aBig && lcaSet(t2, A) == mu)
                return pick(A, C);
            if (cBig && lcaSet(t2, C) == mu)
                return pick(C, A);
        }
        if (aBig)
            return pick(A, C);
        if (cBig)
            return pick(C, A);
        pick(A.size() >= C.size() ? A : C, A.size() >= C.size() ? C : A);
    }

    // ---- final triplet procedures ----
    void proc4a(Label r1, Label r2, Label b)
    {
        if (not restrictsO(t2o, b, r1, r2))
            std::swap(r1, r2);  // ensure b r1 | r2 in original T2
        Node* m = lca(t2.node(r1), t2.node(b));
        if (m != nullptr && m->parent != nullptr)
            cutEdge(t2, m);
        dualSum -= 1;  // y_{lca2(r1,b)} <- ... - 1
        cutOffLeaf(t2, r2);
        cutOffLeaf(t1, r2);
        deactivate(r2);
        dualSum += 1;  // y_r2 <- 1
        if (activeSiblings(t2, r1, b))
        {
            deactivate(b);
            dualSum += 1;  // merge b into r1, y_b <- 1
        }
        else
        {
            cutOffLeaf(t2, r1);
            cutOffLeaf(t1, r1);
            deactivate(r1);
            cutOffLeaf(t2, b);
            cutOffLeaf(t1, b);
            deactivate(b);
            dualSum += 2;  // y_r1 <- 1, y_b <- 1
        }
    }

    void proc4b(Label r1, Label r2, Label b)
    {
        cutOffLeaf(t2, b);
        cutOffLeaf(t1, b);
        deactivate(b);
        dualSum += 1;  // y_b <- 1
        for (Label r : {r1, r2})
        {
            if (not aloneInTree(t2, r))
                cutOffLeaf(t2, r);
            cutOffLeaf(t1, r);
            deactivate(r);
            dualSum += 1;  // y_r <- 1
        }
        // retroactive merge (line 76): skipped (no y change)
    }

    void proc4c(Label r1, Label r2, Label b)
    {
        Label trip[3] = {r1, r2, b};
        Label uHat = 0;
        for (Label x : trip)
        {
            Label o1 = 0, o2 = 0;
            for (Label y : trip)
                if (y != x)
                    (o1 == 0 ? o1 : o2) = y;
            if (not sameTree(t2, x, o1) && not sameTree(t2, x, o2))
            {
                uHat = x;
                break;
            }
        }
        if (uHat == 0)
            for (Label x : trip)
                if (aloneInTree(t2, x))
                {
                    uHat = x;
                    break;
                }
        if (uHat == 0)
            uHat = b;
        Label v1 = 0, v2 = 0;
        for (Label x : trip)
            if (x != uHat)
                (v1 == 0 ? v1 : v2) = x;
        const bool line80 = aloneInTree(t2, uHat);
        if (line80)
            cutOffLeaf(t1, uHat);
        else
        {
            cutOffLeaf(t2, uHat);
            cutOffLeaf(t1, uHat);
        }
        deactivate(uHat);
        dualSum += 1;  // y_uHat <- 1
        if (activeSiblings(t2, v1, v2))
        {
            if (v2 == b)
                std::swap(v1, v2);  // relabel so v1 = b (Claim 5)
            deactivate(v1);
            dualSum += 1;  // merge, y_b <- 1
        }
        else
        {
            resolvePair(v1, v2, {v1, v2});
            std::vector<Label> last;
            for (Label x : {v1, v2})
                if (active.contains(x))
                    last.push_back(x);
            if (not last.empty())
            {
                Label lv = last[0];
                if (not aloneInTree(t2, lv))
                    cutOffLeaf(t2, lv);
                cutOffLeaf(t1, lv);
                deactivate(lv);
                dualSum += 1;  // y_v2 <- 1
            }
        }
        // retroactive merge (line 91): skipped (no y change)
    }

    // ---- Preprocess (Procedure 3) ----
    void preprocess()
    {
        bool changed = true;
        while (changed)
        {
            if (timedOut())
            {
                aborted = true;
                return;
            }
            changed = false;
            // 1. merge an active sibling pair that is a sibling pair in BOTH forests (no dual change).
            auto [u, v] = findBothForestCherry();
            if (u != 0)
            {
                deactivate(u);
                changed = true;
                continue;
            }
            // 2. else finalise a leaf that is the only active leaf in its T2 tree.
            const Label w = findT2Alone();
            if (w != 0)
            {
                cutOffLeaf(t1, w);
                deactivate(w);
                dualSum += 1;  // y_w <- 1
                changed = true;
            }
        }
    }

    // ---- main loop (Algorithm 2) ----
    long run()
    {
        preprocess();
        long guard = 0;
        while (not active.empty())
        {
            if (++guard > 2000000)
                break;
            if (timedOut())
            {
                aborted = true;
                break;
            }
            // Termination. Neither branch credits a y. The last surviving component is deliberately
            // never given a dual variable, because the bound is L = D+1 with D = sum(y)-1, so the "+1"
            // of that conversion already is that component. Crediting it here as well counts it twice
            // and made this dual return L = k*+1 (see res/lowerbound/README.md). The paper states the
            // same restriction only in passing: its preprocessing gives a unit of dual to a leaf that
            // is alone in its T2 tree, but only when that leaf "is not the last active leaf".
            if (active.size() == 1)
            {
                active.clear();  // the last active leaf is the final component
                break;
            }
            std::vector<Label> R, B;
            if (not findMinIncompat(R, B))
            {
                active.clear();  // remaining active leaves are mutually compatible -> the final component
                break;
            }
            if (not resolveSet(R))  // Fail
            {
                // line 46: y_{lca1(RuB)} <- -1, y_{lca1(R)} <- +1  (net 0)
                const std::unordered_set<Label> Bset(B.begin(), B.end());
                while (static_cast<int>(activeIn(B).size()) >= 2)
                {
                    auto [a, b] = activeSiblingPairIn(B);
                    if (a == 0)
                        break;
                    resolvePair(a, b, Bset);
                }
                std::vector<Label> remR = activeIn(R);
                std::vector<Label> remB = activeIn(B);
                if (remR.size() == 2 && remB.size() == 1)
                {
                    Label r1 = remR[0], r2 = remR[1], b = remB[0];
                    std::unordered_set<Node*> roots{t2.rootOf(t2.node(r1)), t2.rootOf(t2.node(r2)),
                                                    t2.rootOf(t2.node(b))};
                    if (roots.size() == 1)
                        proc4a(r1, r2, b);
                    else if (roots.size() == 3)
                        proc4b(r1, r2, b);
                    else
                        proc4c(r1, r2, b);
                }
                else
                {
                    // degenerate remainder: finalize remaining active in R u B safely (each +1)
                    std::vector<Label> ru = R;
                    ru.insert(ru.end(), B.begin(), B.end());
                    for (Label x : ru)
                        if (active.contains(x))
                        {
                            if (not aloneInTree(t2, x))
                                cutOffLeaf(t2, x);
                            cutOffLeaf(t1, x);
                            deactivate(x);
                            dualSum += 1;
                        }
                }
            }
            preprocess();
        }
        return dualSum > 1 ? dualSum : 1;
    }
};
}  // namespace

namespace
{
/// The Red-Blue dual bound for the ordered forest pair (f1, f2). Cutting edges in f2 is charged, so the
/// value can differ per ordering; callers take the max over orderings for the tightest valid bound.
/// Returns -1 if the deadline was hit before finishing (the partial dual sum is not a valid bound).
long redBlueTwoForest(const graph::Forest& f1, const graph::Forest& f2, solver::Deadline deadline,
                      const std::atomic<bool>* stop)
{
    MutableForestView t1w(f1), t2w(f2), t1o(f1), t2o(f2);
    ActiveSet active;
    active.reserve(t1w.nodeOf.size());
    for (const auto& [label, node] : t1w.nodeOf)
        active.insert(label);
    RedBlue2 rb(t1w, t2w, t1o, t2o, active, deadline, stop);
    const long value = rb.run();
    return rb.aborted ? -1 : value;
}
}  // namespace

long solver::computeDual2ApproxLowerBound(const graph::Instance& instance, Deadline deadline,
                                          const std::atomic<bool>* stop)
{
    // A maximum agreement forest of all input forests is also an agreement forest of any pair of them,
    // so k*(all) >= k*(pair) >= L2(pair): the max over pairs is a valid lower bound for the full instance.
    const std::size_t k = instance.size();
    if (k < 2)
        return 1;
    long best = 0;  // 0 = no pair finished in time (the "did not finish" sentinel)
    for (std::size_t i = 0; i < k; ++i)
        for (std::size_t j = i + 1; j < k; ++j)
            for (auto [a, b] : {std::pair{i, j}, std::pair{j, i}})  // both orderings
            {
                const long r = redBlueTwoForest(*instance.at(a), *instance.at(b), deadline, stop);
                if (r >= 0)  // finished in time -> a valid bound
                    best = std::max(best, r);
            }
    return best;
}

long solver::computeCertifiedLowerBound(const graph::Instance& instance, Deadline deadline)
{
    // Always the fast 3-approx; add the tighter 2-approx only if it finishes within the deadline (it
    // returns 0 otherwise). Both are valid (<= k*), so the maximum is valid and never below the 3-approx.
    const long l3 = computeDual3ApproxLowerBound(instance);
    const long l2 = computeDual2ApproxLowerBound(instance, deadline);
    return l2 > l3 ? l2 : l3;
}
