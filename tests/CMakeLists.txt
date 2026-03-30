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

add_executable(BranchingSolverSimpleTests BranchingSolverSimpleTests.cpp)
target_link_libraries(BranchingSolverSimpleTests PRIVATE Solver Graph Catch2::Catch2WithMain)
add_test(NAME BranchingSolverSimpleTests COMMAND $<TARGET_FILE:BranchingSolverSimpleTests>)
target_compile_definitions(BranchingSolverSimpleTests PRIVATE RES_DIR="${CMAKE_SOURCE_DIR}/res/")

add_subdirectory(ILP)