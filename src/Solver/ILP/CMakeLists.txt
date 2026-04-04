file(GLOB ilp_source *.cpp)
file(GLOB ilp_header *.hpp)

# Interfaces Ordner
file(GLOB interfaces_source Interfaces/*.cpp)
file(GLOB interfaces_header Interfaces/*.hpp)

add_library(ILP
    ${ilp_source}
    ${interfaces_source}
)

target_link_libraries(ILP Graph Cluster EvalMaxSAT)