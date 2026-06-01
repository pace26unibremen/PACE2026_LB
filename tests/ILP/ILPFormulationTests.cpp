#include <catch2/catch_test_macros.hpp>

#include "../../src/Graph/Instance.hpp"
#include "../../src/Solver/ILP/ILPFormulation.hpp"
#include "../../src/Solver/ILP/ILPConstraintUtils.hpp"
#include "../../src/Solver/ILP/TreeUtils.hpp"
#include "../../src/Cluster/LeastCommonAncestor.hpp"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Loads a two-tree instance and returns both forests.
static std::pair<std::shared_ptr<graph::Forest>, std::shared_ptr<graph::Forest>>
loadInstance(const std::string& filename)
{
    auto instance = graph::ReadInstance(std::string(ILP_TEST_DIR) + filename);
    return { (*instance)[0], (*instance)[1] };
}

/// Builds all inputs needed for constraint computation from two forests.
struct ConstraintTestFixture
{
    std::shared_ptr<graph::Forest> f1;
    std::shared_ptr<graph::Forest> f2;
    cluster::LeastCommonAncestor lca1;
    cluster::LeastCommonAncestor lca2;
    std::unordered_map<const graph::Node*, int> map1;
    std::unordered_map<const graph::Node*, int> map2;
    std::unordered_map<uint64_t, std::vector<int>> pathCache1;
    std::unordered_map<uint64_t, std::vector<int>> pathCache2;

    explicit ConstraintTestFixture(const std::string& filename)
        : f1(loadInstance(filename).first)
        , f2(loadInstance(filename).second)
        , lca1(f1)
        , lca2(f2)
        , map1(solver::buildNodeToIndexMap(*f1))
        , map2(solver::buildNodeToIndexMap(*f2))
    {}
};

// ---------------------------------------------------------------------------
// computeTripleConstraints
// ---------------------------------------------------------------------------

TEST_CASE("computeTripleConstraints", "[ILPConstraintUtils]")
{
    SECTION("identical_4leaves: no constraints expected")
    {
        ConstraintTestFixture fix("ilp_identical_4leaves.nw");

        auto constraints = solver::computeTripleConstraints(
            *fix.f1, *fix.f2, fix.lca1, fix.lca2, fix.map1, fix.map2, fix.pathCache1);

        INFO("identical trees must produce no triple constraints");
        REQUIRE(constraints.empty());
    }

    SECTION("single_incompatible_triple: exactly one constraint expected")
    {
        ConstraintTestFixture fix("ilp_single_incompatible_triple.nw");

        auto constraints = solver::computeTripleConstraints(
            *fix.f1, *fix.f2, fix.lca1, fix.lca2, fix.map1, fix.map2, fix.pathCache1);

        INFO("exactly one incompatible triple must produce exactly one constraint");
        REQUIRE(constraints.size() == 1);

        INFO("size of that constraint should be 4");
        REQUIRE(constraints[0].size() == 4);
    }

    SECTION("structural properties hold for all constraints")
    {
        ConstraintTestFixture fix("ilp_asymmetric_4leaves.nw");
        int nEdges = static_cast<int>(fix.f1->Nodes().size()) - 1;

        auto constraints = solver::computeTripleConstraints(
            *fix.f1, *fix.f2, fix.lca1, fix.lca2, fix.map1, fix.map2, fix.pathCache1);

        for (const auto& constraint : constraints)
        {
            INFO("each constraint must be non-empty");
            REQUIRE_FALSE(constraint.empty());

            INFO("all variable indices must be in valid range");
            for (int idx : constraint)
            {
                REQUIRE(idx >= 0);
                REQUIRE(idx <= nEdges);
            }
        }
    }

    SECTION("caterpillar_5leaves: expected count from old implementation")
    {
        ConstraintTestFixture fix("ilp_caterpillar_5leaves.nw");

        auto constraints = solver::computeTripleConstraints(
            *fix.f1, *fix.f2, fix.lca1, fix.lca2, fix.map1, fix.map2, fix.pathCache1);

        REQUIRE(constraints.size() == 9);
        for (const auto& constraint : constraints)
        {
            REQUIRE(constraint.size() >=4);
            REQUIRE(constraint.size() <= 6);
        }
    }
}

// ---------------------------------------------------------------------------
// computePathPairConstraints
// ---------------------------------------------------------------------------

TEST_CASE("computePathPairConstraints", "[ILPConstraintUtils]")
{
    SECTION("identical_4leaves: no constraints expected")
    {
        ConstraintTestFixture fix("ilp_identical_4leaves.nw");

        auto constraints = solver::computePathPairConstraints(
            *fix.f1, *fix.f2, fix.lca1, fix.lca2, fix.map1, fix.map2, fix.pathCache1, fix.pathCache2);

        INFO("identical trees must produce no path-pair constraints");
        REQUIRE(constraints.empty());
    }

    SECTION("disjoint_paths_4leaves: exactly one constraint expected")
    {
        ConstraintTestFixture fix("ilp_disjoint_paths_4leaves.nw");

        auto constraints = solver::computePathPairConstraints(
            *fix.f1, *fix.f2, fix.lca1, fix.lca2, fix.map1, fix.map2, fix.pathCache1, fix.pathCache2);

        INFO("disjoint paths instance must produce at least one path-pair constraint");
        REQUIRE_FALSE(constraints.empty());
        REQUIRE(constraints.size() == 1);
        INFO("size of that constraint should be 4");
        REQUIRE(constraints[0].size() == 4);

    }

    SECTION("structural properties hold for all constraints")
    {
        ConstraintTestFixture fix("ilp_caterpillar_5leaves.nw");
        int nEdges = static_cast<int>(fix.f1->Nodes().size()) - 1;

        auto constraints = solver::computePathPairConstraints(
            *fix.f1, *fix.f2, fix.lca1, fix.lca2, fix.map1, fix.map2, fix.pathCache1, fix.pathCache2);

        for (const auto& constraint : constraints)
        {
            INFO("each path-pair constraint must contain at least 2 variables");
            REQUIRE(constraint.size() >= 2);

            INFO("all variable indices must be in valid range");
            for (int idx : constraint)
            {
                REQUIRE(idx >= 0);
                REQUIRE(idx <= nEdges);
            }
        }
    }

    SECTION("caterpillar_5leaves: expected count from old implementation")
    {
        ConstraintTestFixture fix("ilp_caterpillar_5leaves.nw");

        auto constraints = solver::computePathPairConstraints(
            *fix.f1, *fix.f2, fix.lca1, fix.lca2, fix.map1, fix.map2, fix.pathCache1, fix.pathCache2);

        REQUIRE(constraints.size() == 4);
        for (const auto& constraint : constraints)
        {
            REQUIRE(constraint.size() >=5);
            REQUIRE(constraint.size() <= 6);
        }
    }
}

// ---------------------------------------------------------------------------
// ILPFormulation::build()
// ---------------------------------------------------------------------------
//#if 0
TEST_CASE("ILPFormulation::build", "[ILPFormulation]")
{
    SECTION("identical_4leaves: structural properties of variables")
    {
        auto [f1, f2] = loadInstance("ilp_identical_4leaves.nw");
        solver::ILPFormulation formulation(f1, f2);
        auto problem = formulation.build();

        INFO("ILP must be a minimization problem");
        REQUIRE(problem.fMinimize);

        int nEdges = static_cast<int>(f1->Nodes().size());
        INFO("number of variables must equal total number of edges in both forests");
        REQUIRE(problem.nVars() == nEdges); 

        std::set<int> seenVarNums;
        for (const auto& var : problem.vars)
        {
            INFO("all variables must be binary");
            REQUIRE(var.isBinary);

            INFO("all objective coefficients must be 1.0");
            REQUIRE(var.objCoeff == 1.0);

            INFO("varNum must be in valid range");
            REQUIRE(var.varNum >= 0);
            REQUIRE(var.varNum < problem.nVars());

            INFO("varNum must be unique");
            REQUIRE(seenVarNums.count(var.varNum) == 0);
            seenVarNums.insert(var.varNum);

            INFO("varName must not be empty");
            REQUIRE(!var.varName.empty());
        }

        INFO("identical trees produce only root constraint");
        REQUIRE(problem.nConstraints() == 1);
    }

    SECTION("asymmetric_4leaves: structural properties of constraints hold")
    {
        auto [f1, f2] = loadInstance("ilp_asymmetric_4leaves.nw");
        solver::ILPFormulation formulation(f1, f2);
        auto problem = formulation.build();

        for (const auto& constraint : problem.constraints)
        {
            INFO("constraint must reference at least one variable");
            REQUIRE(!constraint.varIndices.empty());

            INFO("all variable indices must be in valid range");
            for (int idx : constraint.varIndices)
            {
                REQUIRE(idx >= 0);
                REQUIRE(idx < problem.nVars());
            }
        }

        int nEdges = static_cast<int>(f1->Nodes().size());
        INFO("number of variables must equal total number of edges in both forests");
        REQUIRE(problem.nVars() == nEdges); // Should be 7...

        int nConstraints = 3;
        INFO("number of constraints should equal 3 [compared with SPRDist-Solver]");
        REQUIRE(problem.nConstraints() == nConstraints);
    }

    SECTION("minimal_2leaves: 2 variables, 0 constraints")
    {
        auto [f1, f2] = loadInstance("ilp_minimal_2leaves.nw");
        solver::ILPFormulation formulation(f1, f2);
        auto problem = formulation.build();
        
        int nEdges = static_cast<int>(f1->Nodes().size());
        INFO("number of variables must equal total number of edges in both forests");
        REQUIRE(problem.nVars() == nEdges); // Should be 2...
 
        INFO("2-leaf trees produce only root constraint");
        REQUIRE(problem.nConstraints() == 1);
    }

    SECTION("single_incompatible_triple: expected constraint count from old implementation")
    {
        auto [f1, f2] = loadInstance("ilp_single_incompatible_triple.nw");
        solver::ILPFormulation formulation(f1, f2);
        auto problem = formulation.build();

        int nEdges = static_cast<int>(f1->Nodes().size());
        INFO("number of variables must equal total number of edges in both forests");
        REQUIRE(problem.nVars() == nEdges); // Should be 5...
        
        int nConstraints = 2;
        INFO("number of constraints should equal 2 [compared with SPRDist-Solver]");
        REQUIRE(problem.nConstraints() == nConstraints);
    }

    SECTION("caterpillar_5leaves: expected constraint count from old implementation")
    {
        auto [f1, f2] = loadInstance("ilp_caterpillar_5leaves.nw");
        solver::ILPFormulation formulation(f1, f2);
        auto problem = formulation.build();

        int nEdges = static_cast<int>(f1->Nodes().size());
        INFO("number of variables must equal total number of edges in both forests");
        REQUIRE(problem.nVars() == nEdges); // Should be 9...
        
        int nConstraints = 14;
        INFO("number of constraints should equal 14 [compared with SPRDist-Solver]");
        REQUIRE(problem.nConstraints() == nConstraints);
    }
}
//#endif