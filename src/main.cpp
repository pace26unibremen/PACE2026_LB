#include "Solver/BranchingSolver.hpp"
#include "Solver/ReductionSolver.hpp"
#include "Graph/Instance.hpp"

#include <iostream>

using namespace std;
using namespace graph;

int main(int, char**)
{
    std::stringstream sstream;
    sstream
     << "#p 2 20" << endl
     << "((3,(((((8,10),11),2),(9,19)),(20,18))),(((((15,4),(13,7)),17),((1,(12,16)),6)),(5,14)));" << endl
     << "((((4,7),5),((6,14),(((12,1),(17,13)),(16,15)))),((((((8,10),11),2),(9,19)),(20,18)),3));" << endl;
    auto i = ReadInstance(sstream);

    auto c = std::make_shared<solver::BranchingSolverConfiguration>();
    c->boundedDephtSearch = true;
    // c->debPlugin = std::make_shared<solver::DebugPlugin>(string(RES_DIR) + "debugPlugin/");
    c->activeRules =
    {
        solver::CutBranchRule::isApplicable,
        solver::EqualForestsRule::isApplicable,
        solver::SingleVertexTreePropagationRule::isApplicable,
        solver::EqualForestsRule::isApplicable,
        solver::ACBranchingRule::isApplicable,
        solver::ABCBranchingRule::isApplicable,
        solver::DebugAssertFalseRule::isApplicable
    };

    auto rs = solver::ReductionSolver(i);
    auto bs = solver::BranchingSolver(i,c);

    rs.solve();
    bs.solve();
    bs.unapplyReductions();
    rs.unapplyReductions();

    DotInstance(i,cout);
    return 17;
}
