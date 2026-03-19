file (GLOB solver_source *.cpp)
file (GLOB solver_header *.hpp)

file(GLOB ilp_source ILP/*.cpp)
file(GLOB ilp_header ILP/*.hpp)

add_library(Solver 
        ${solver_source}
        ${ilp_source}
        Rule/ChainReductionRule.cpp
        Rule/ChainReductionRule.h)
target_link_libraries(Solver Graph Cluster)

set_target_properties(Solver PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
        PUBLIC_HEADER "${solver_header}")

add_subdirectory(Action)
add_subdirectory(Rule)
add_subdirectory(ILP)