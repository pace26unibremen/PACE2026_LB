file(GLOB ilp_source *.cpp)
file(GLOB ilp_header *.hpp)

add_library(ILP_EVALMAXSAT ${ilp_source} Interfaces/EvalMaxSATSolver.cpp)
target_link_libraries(ILP_EVALMAXSAT EvalMaxSAT)
target_compile_definitions(ILP_EVALMAXSAT PRIVATE USE_EVALMAXSAT)
target_include_directories(ILP_EVALMAXSAT PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/EvalMaxSAT/lib/EvalMaxSAT/src
    ${CMAKE_SOURCE_DIR}/lib/EvalMaxSAT/lib/MaLib/src
    ${CMAKE_SOURCE_DIR}/lib/EvalMaxSAT/lib/cadical/src
)

if(SCIP_FOUND)
    add_library(ILP_SCIP ${ilp_source} Interfaces/SCIPSolver.cpp)
    target_link_libraries(ILP_SCIP ${SCIP_LIBRARIES})
    target_compile_definitions(ILP_SCIP PRIVATE USE_SCIP)
    target_include_directories(ILP_SCIP PRIVATE ${SCIP_INCLUDE_DIRS})
endif()