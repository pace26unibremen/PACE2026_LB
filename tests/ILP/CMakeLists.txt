add_executable(TreeUtilsTests TreeUtilsTests.cpp)
target_link_libraries(TreeUtilsTests PRIVATE Solver Graph Cluster Catch2::Catch2WithMain)
add_test(NAME TreeUtilsTests COMMAND $<TARGET_FILE:TreeUtilsTests>)
target_compile_definitions(TreeUtilsTests PRIVATE
    ILP_TEST_DIR="${CMAKE_SOURCE_DIR}/res/tests/ilp/")

add_executable(ILPFormulationTests ILPFormulationTests.cpp)
target_link_libraries(ILPFormulationTests PRIVATE Solver Graph Cluster Catch2::Catch2WithMain)
add_test(NAME ILPFormulationTests COMMAND $<TARGET_FILE:ILPFormulationTests>)
target_compile_definitions(ILPFormulationTests PRIVATE
    ILP_TEST_DIR="${CMAKE_SOURCE_DIR}/res/tests/ilp/")