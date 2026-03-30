add_subdirectory(Graph)
add_subdirectory(Cluster)
add_subdirectory(Solver)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE Solver Graph Cluster)
target_compile_definitions(main PRIVATE RES_DIR="${CMAKE_SOURCE_DIR}/res/")

add_executable(solver_exec solver.cpp)
target_link_libraries(solver_exec PRIVATE Solver Graph Cluster)
set_target_properties(solver_exec PROPERTIES 
    OUTPUT_NAME "solver"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)