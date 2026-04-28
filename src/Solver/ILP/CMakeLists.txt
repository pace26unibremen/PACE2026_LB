file(GLOB ilp_source *.cpp)
file(GLOB ilp_header *.hpp)
list(FILTER ilp_source EXCLUDE REGEX ".*MAFILPSolver\\.cpp$")

# EvalMaxSAT
add_library(ILP_EVALMAXSAT ${ilp_source} MAFILPSolver.cpp Interfaces/EvalMaxSATSolver.cpp)
target_link_libraries(ILP_EVALMAXSAT Solver Graph Cluster EvalMaxSAT)
target_compile_definitions(ILP_EVALMAXSAT PRIVATE USE_EVALMAXSAT)
target_include_directories(ILP_EVALMAXSAT PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/EvalMaxSAT/lib/EvalMaxSAT/src
    ${CMAKE_SOURCE_DIR}/lib/EvalMaxSAT/lib/MaLib/src
    ${CMAKE_SOURCE_DIR}/lib/EvalMaxSAT/lib/cadical/src
)

# UWrMaxSat
# add_library(ILP_UWRMAXSAT ${ilp_source} MAFILPSolver.cpp Interfaces/UWrMaxSatSolver.cpp)
# target_link_libraries(ILP_UWRMAXSAT Solver Graph Cluster UWrMaxSat)
# target_compile_definitions(ILP_UWRMAXSAT PRIVATE USE_UWRMAXSAT)
# target_include_directories(ILP_UWRMAXSAT PRIVATE
#    ${CMAKE_SOURCE_DIR}/lib/UWrMaxSat/uwrmaxsat
#    ${CMAKE_SOURCE_DIR}/lib/UWrMaxSat/uwrmaxsat/ADTs
# )

# SCIP
if(SCIP_FOUND)
    add_library(ILP_SCIP ${ilp_source} MAFILPSolver.cpp Interfaces/SCIPSolver.cpp)
    target_link_libraries(ILP_SCIP Solver Graph Cluster ${SCIP_LIBRARIES})
    target_compile_definitions(ILP_SCIP PRIVATE USE_SCIP)
    target_include_directories(ILP_SCIP PRIVATE ${SCIP_INCLUDE_DIRS})
endif()