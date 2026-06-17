add_executable(TreeUtilsTests TreeUtilsTests.cpp)
target_link_libraries(TreeUtilsTests PRIVATE ILP_EVALMAXSAT Solver Graph Cluster Catch2::Catch2WithMain)
add_test(NAME TreeUtilsTests COMMAND $<TARGET_FILE:TreeUtilsTests>)
target_compile_definitions(TreeUtilsTests PRIVATE
    ILP_TEST_DIR="${CMAKE_SOURCE_DIR}/res/tests/ilp/")

add_executable(ILPFormulationTests ILPFormulationTests.cpp)
target_link_libraries(ILPFormulationTests PRIVATE ILP_EVALMAXSAT Solver Graph Cluster Catch2::Catch2WithMain)
add_test(NAME ILPFormulationTests COMMAND $<TARGET_FILE:ILPFormulationTests>)
target_compile_definitions(ILPFormulationTests PRIVATE
    ILP_TEST_DIR="${CMAKE_SOURCE_DIR}/res/tests/ilp/")

# EvalMaxSATTest
add_executable(ILPSolverEvalMaxSATTests ILPSolverEvalMaxSATTests.cpp)
target_link_libraries(ILPSolverEvalMaxSATTests PRIVATE 
    Solver ILP_EVALMAXSAT Graph Cluster Catch2::Catch2WithMain)
target_compile_definitions(ILPSolverEvalMaxSATTests PRIVATE 
    USE_EVALMAXSAT
    ILP_TEST_DIR="${CMAKE_SOURCE_DIR}/res/tests/ilp/")
add_test(NAME ILPSolverEvalMaxSATTests COMMAND $<TARGET_FILE:ILPSolverEvalMaxSATTests>)

# UWrMaxSAT Test
if(BUILD_UWRMAXSAT)
    add_executable(ILPSolverUWrMaxSatTests ILPSolverUWrMaxSatTests.cpp)
    target_link_libraries(ILPSolverUWrMaxSatTests PRIVATE
        Solver ILP_UWRMAXSAT Graph Cluster Catch2::Catch2WithMain)
    target_compile_definitions(ILPSolverUWrMaxSatTests PRIVATE
        USE_UWRMAXSAT
        ILP_TEST_DIR="${CMAKE_SOURCE_DIR}/res/tests/ilp/")
    if(USE_MAXPRE)
        target_compile_definitions(ILP_UWRMAXSAT PRIVATE USE_MAXPRE)
        target_include_directories(ILP_UWRMAXSAT PRIVATE 
            ${CMAKE_SOURCE_DIR}/lib/UWrMaxSat/maxpre/src)
    endif()
    add_test(NAME ILPSolverUWrMaxSatTests COMMAND $<TARGET_FILE:ILPSolverUWrMaxSatTests>)
endif()

# ScipTest
if(SCIP_FOUND)
    add_executable(ILPSolverSCIPTests ILPSolverSCIPTests.cpp)
    target_link_libraries(ILPSolverSCIPTests PRIVATE 
        Solver ILP_SCIP Graph Cluster Catch2::Catch2WithMain)
    target_compile_definitions(ILPSolverSCIPTests PRIVATE 
        USE_SCIP
        ILP_TEST_DIR="${CMAKE_SOURCE_DIR}/res/tests/ilp/")
    add_test(NAME ILPSolverSCIPTests COMMAND $<TARGET_FILE:ILPSolverSCIPTests>)
endif()

# IncrementalSolver Tests
# Incremental UWrMaxSAT Test
if(BUILD_INCREMENTAL)
    add_executable(IncrementalSolverTests IncrementalSolverTests.cpp)
    target_link_libraries(IncrementalSolverTests PRIVATE
        Solver ILP_INCREMENTAL Graph Cluster Catch2::Catch2WithMain)
    target_compile_definitions(IncrementalSolverTests PRIVATE
        USE_UWRMAXSAT
        USE_INCR
        ILP_TEST_DIR="${CMAKE_SOURCE_DIR}/res/tests/ilp/")
    add_test(NAME IncrementalSolverTests COMMAND $<TARGET_FILE:IncrementalSolverTests>)
endif()