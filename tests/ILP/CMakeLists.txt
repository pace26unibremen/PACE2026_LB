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

if(SCIP_FOUND)
    add_executable(ILPSolverSimpleTests ILPSolverSimpleTests.cpp)
    target_link_libraries(ILPSolverSimpleTests PRIVATE 
        Solver
        ILP_EVALMAXSAT 
        ILP_SCIP
        Graph Cluster Catch2::Catch2WithMain)
    target_compile_definitions(ILPSolverSimpleTests PRIVATE 
        USE_EVALMAXSAT 
        USE_SCIP
        ILP_TEST_DIR="${CMAKE_SOURCE_DIR}/res/tests/ilp/")
    add_test(NAME ILPSolverSimpleTests COMMAND $<TARGET_FILE:ILPSolverSimpleTests>)
else()
    add_executable(ILPSolverSimpleTests ILPSolverSimpleTests.cpp)
    target_link_libraries(ILPSolverSimpleTests PRIVATE 
        ILP_EVALMAXSAT
        Graph Cluster Catch2::Catch2WithMain)
    target_compile_definitions(ILPSolverSimpleTests PRIVATE 
        USE_EVALMAXSAT
        ILP_TEST_DIR="${CMAKE_SOURCE_DIR}/res/tests/ilp/")
    add_test(NAME ILPSolverSimpleTests COMMAND $<TARGET_FILE:ILPSolverSimpleTests>)
endif()