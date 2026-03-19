#include "Solver/BranchingSolver.hpp"
#include <fstream>
#include <iostream>

void runOnStream(std::istream& inStream, std::ostream& outStream) {
    // Start Timer
    auto startTime = std::clock();
    auto instance = graph::ReadInstance(inStream);
    auto solver = solver::BranchingSolver(instance);
    auto solution = solver.solve();
    auto endTime = std::clock();
    auto time_delta_ms = ((double) (endTime - startTime)) / ((double) CLOCKS_PER_SEC / 1000.0);
    outStream << "# t " << time_delta_ms << "\n# s " << solution->Roots().size() << "\n";
    solution->write(outStream);
}

int main(int argc, char* argv[]) {
    if (argc == 1)
    {
        // With no arguments, read from stdin and write to stdout for stride benchmark
        runOnStream(std::cin, std::cout);
        return 0;
    }

    // Else run with files
    std::string infile;
    std::string outfile;

    if (argc == 2)
    {
        infile = argv[1];
        outfile = infile + "_solution.tree";
    }
    else if (argc == 3)
    {
        infile = argv[1];
        outfile = argv[2];
    }
    else
    {
        throw std::invalid_argument("Wrong number of arguments");
    }

    std::ifstream inStream(infile);
    std::ofstream outStream(outfile);
    runOnStream(inStream, outStream);

    return 0;
}
