#find_package(Catch2)
if(NOT Catch2_FOUND)
    include(FetchContent)
    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.11.0
    )
    FetchContent_MakeAvailable(Catch2)
endif()
include(CTest)
include(CPack)
enable_testing()

add_executable(ForestIOTests ForestIOTests.cpp)
target_link_libraries(ForestIOTests PRIVATE Graph Catch2::Catch2WithMain)
add_test(NAME ForestIOTests COMMAND $<TARGET_FILE:ForestIOTests>)
target_compile_definitions(ForestIOTests PRIVATE TEST_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/res/tests/")

add_executable(ForestEqualTests ForestEqualTests.cpp)
target_link_libraries(ForestEqualTests PRIVATE Graph Catch2::Catch2WithMain)
add_test(NAME ForestEqualTests COMMAND $<TARGET_FILE:ForestEqualTests>)
target_compile_definitions(ForestEqualTests PRIVATE TEST_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/res/tests/")

add_executable(ForestSubforestTests ForestSubforestTests.cpp)
target_link_libraries(ForestSubforestTests PRIVATE Graph Catch2::Catch2WithMain)
add_test(NAME ForestSubforestTests COMMAND $<TARGET_FILE:ForestSubforestTests>)
target_compile_definitions(ForestSubforestTests PRIVATE TEST_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/res/tests/")

add_executable(DeleteEdgeActionTests DeleteEdgeActionTests.cpp)
target_link_libraries(DeleteEdgeActionTests PRIVATE Solver Graph Catch2::Catch2WithMain)
add_test(NAME DeleteEdgeActionTests COMMAND $<TARGET_FILE:DeleteEdgeActionTests>)
target_compile_definitions(DeleteEdgeActionTests PRIVATE TEST_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/res/tests/")

add_executable(PluginTests PluginTests.cpp)
target_link_libraries(PluginTests PRIVATE Solver Graph Catch2::Catch2WithMain)
add_test(NAME PluginTests COMMAND $<TARGET_FILE:PluginTests>)
target_compile_definitions(PluginTests PRIVATE RES_DIR="${CMAKE_SOURCE_DIR}/res/")

add_executable(MetricsPluginTests MetricsPluginTests.cpp)
target_link_libraries(MetricsPluginTests PRIVATE Solver Graph Catch2::Catch2WithMain)
add_test(NAME MetricsPluginTests COMMAND $<TARGET_FILE:MetricsPluginTests>)

add_executable(BranchingSolverSimpleTests BranchingSolverSimpleTests.cpp)
target_link_libraries(BranchingSolverSimpleTests PRIVATE Solver Graph Catch2::Catch2WithMain)
add_test(NAME BranchingSolverSimpleTests COMMAND $<TARGET_FILE:BranchingSolverSimpleTests>)
target_compile_definitions(BranchingSolverSimpleTests PRIVATE RES_DIR="${CMAKE_SOURCE_DIR}/res/")

add_executable(BranchingSolverPauseResumeTests BranchingSolverPauseResumeTests.cpp)
target_link_libraries(BranchingSolverPauseResumeTests PRIVATE Solver Graph Catch2::Catch2WithMain)
add_test(NAME BranchingSolverPauseResumeTests COMMAND $<TARGET_FILE:BranchingSolverPauseResumeTests>)

add_executable(TwoBRuleTests TwoBRuleTests.cpp)
target_link_libraries(TwoBRuleTests PRIVATE Solver Graph Catch2::Catch2WithMain)
add_test(NAME TwoBRuleTests COMMAND $<TARGET_FILE:TwoBRuleTests>)
target_compile_definitions(TwoBRuleTests PRIVATE RES_DIR="${CMAKE_SOURCE_DIR}/res/")
target_compile_definitions(TwoBRuleTests PRIVATE TEST_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/res/tests/")

add_executable(ReverseBRuleTests ReverseBRuleTests.cpp)
target_link_libraries(ReverseBRuleTests PRIVATE Solver Graph Catch2::Catch2WithMain)
add_test(NAME ReverseBRuleTests COMMAND $<TARGET_FILE:ReverseBRuleTests>)
target_compile_definitions(ReverseBRuleTests PRIVATE RES_DIR="${CMAKE_SOURCE_DIR}/res/")
target_compile_definitions(ReverseBRuleTests PRIVATE TEST_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/res/tests/")

add_executable(ShortenChainActionTests ShortenChainActionTests.cpp)
target_link_libraries(ShortenChainActionTests PRIVATE Solver Graph Catch2::Catch2WithMain)
add_test(NAME ShortenChainActionTests COMMAND $<TARGET_FILE:ShortenChainActionTests>)
target_compile_definitions(ShortenChainActionTests PRIVATE RES_DIR="${CMAKE_SOURCE_DIR}/res/")
target_compile_definitions(ShortenChainActionTests PRIVATE TEST_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/res/tests/")

add_executable(ChainReductionTests ChainReductionTests.cpp)
target_link_libraries(ChainReductionTests PRIVATE Solver Graph Catch2::Catch2WithMain)
add_test(NAME ChainReductionTests COMMAND $<TARGET_FILE:ChainReductionTests>)
target_compile_definitions(ChainReductionTests PRIVATE RES_DIR="${CMAKE_SOURCE_DIR}/res/")
target_compile_definitions(ChainReductionTests PRIVATE TEST_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/res/tests/")

add_executable(ReductionSolverTests ReductionSolverTests.cpp)
target_link_libraries(ReductionSolverTests PRIVATE Solver Graph Catch2::Catch2WithMain)
add_test(NAME ReductionSolverTests COMMAND $<TARGET_FILE:ReductionSolverTests>)

add_executable(ForcedCherryCutRuleTests ForcedCherryCutRuleTests.cpp)
target_link_libraries(ForcedCherryCutRuleTests PRIVATE Solver Graph Catch2::Catch2WithMain)
add_test(NAME ForcedCherryCutRuleTests COMMAND $<TARGET_FILE:ForcedCherryCutRuleTests>)

add_executable(ApproximationTests ApproximationTests.cpp)
target_link_libraries(ApproximationTests PRIVATE Solver Graph Catch2::Catch2WithMain)
add_test(NAME ApproximationTests COMMAND $<TARGET_FILE:ApproximationTests>)
target_compile_definitions(ApproximationTests PRIVATE RES_DIR="${CMAKE_SOURCE_DIR}/res/")

add_executable(DualLowerBoundTests DualLowerBoundTests.cpp)
target_link_libraries(DualLowerBoundTests PRIVATE Solver Graph Catch2::Catch2WithMain)
add_test(NAME DualLowerBoundTests COMMAND $<TARGET_FILE:DualLowerBoundTests>)
target_compile_definitions(DualLowerBoundTests PRIVATE RES_DIR="${CMAKE_SOURCE_DIR}/res/")

add_executable(ApproximationConfigTests ApproximationConfigTests.cpp)
target_link_libraries(ApproximationConfigTests PRIVATE Solver Graph Catch2::Catch2WithMain)
add_test(NAME ApproximationConfigTests COMMAND $<TARGET_FILE:ApproximationConfigTests>)

add_executable(GreedyCherryCutRuleTests GreedyCherryCutRuleTests.cpp)
target_link_libraries(GreedyCherryCutRuleTests PRIVATE Solver Graph Catch2::Catch2WithMain)
add_test(NAME GreedyCherryCutRuleTests COMMAND $<TARGET_FILE:GreedyCherryCutRuleTests>)
target_compile_definitions(GreedyCherryCutRuleTests PRIVATE RES_DIR="${CMAKE_SOURCE_DIR}/res/")

#add_executable(ClusterPointTests ClusterPointTests.cpp)
#target_link_libraries(ClusterPointTests PRIVATE Solver Graph Cluster Catch2::Catch2WithMain)
#add_test(NAME ClusterPointTests COMMAND $<TARGET_FILE:ClusterPointTests>)
#target_compile_definitions(ClusterPointTests PRIVATE TEST_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/res/tests/")
#
#add_executable(MiscellaneousTests MiscellaneousTests.cpp)
#target_link_libraries(MiscellaneousTests PRIVATE Solver Graph Cluster Catch2::Catch2WithMain)
#add_test(NAME MiscellaneousTests COMMAND $<TARGET_FILE:MiscellaneousTests>)
#target_compile_definitions(MiscellaneousTests PRIVATE TEST_EXAMPLES_DIR="${CMAKE_SOURCE_DIR}/res/tests/")

#add_subdirectory(ILP)