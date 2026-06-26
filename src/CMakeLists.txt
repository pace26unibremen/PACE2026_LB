add_subdirectory(Graph)
add_subdirectory(Cluster)
add_subdirectory(Solver)

# Shared macro for solver targets
macro(add_solver_target TARGET_NAME DEFINITIONS LINK_LIBS)
    add_executable(${TARGET_NAME} startSolver.cpp)
    target_compile_definitions(${TARGET_NAME} PRIVATE ${DEFINITIONS})
    target_link_libraries(${TARGET_NAME} PRIVATE ${LINK_LIBS})
    set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src"
    )
endmacro()
    
# Target 1: BranchingSolver
# Note we still keep the name "startSolver" for backward compatibility
add_solver_target(startSolver "" "Solver;Graph;Cluster")
if(PIPELINE_BUILD)
    set_target_properties(startSolver PROPERTIES
        OUTPUT_NAME "startSolver" # Comment out for building on macos
    )
endif()

# Target 2: EvalMaxSAT
add_solver_target(solver_evalmaxsat "USE_EVALMAXSAT" "Solver;ILP_EVALMAXSAT;Graph;Cluster")

# Target 3: UWrMaxSat
if(BUILD_UWRMAXSAT)
    add_solver_target(solver_uwrmaxsat "USE_UWRMAXSAT" "Solver;ILP_UWRMAXSAT;Graph;Cluster")
endif()
# Target 3: SCIP
if(SCIP_FOUND)
    add_solver_target(solver_scip "USE_SCIP" "Solver;ILP_SCIP;Graph;Cluster")
endif()

#Target 4: Incremental Solver
if(BUILD_INCREMENTAL_UWRMAXSAT)
    add_solver_target(solver_incremental_uwrmaxsat 
        "USE_UWRMAXSAT;USE_INCR" 
        "Solver;ILP_INCREMENTAL_UWRMAXSAT;Graph;Cluster")
endif()

# Target 5: Incremental EvalMaxSAT
if(BUILD_INCREMENTAL_EVALMAXSAT)
    add_solver_target(solver_incremental_evalmaxsat
        "USE_EVALMAXSAT;USE_INCR"
        "Solver;ILP_INCREMENTAL_EVALMAXSAT;Graph;Cluster")
endif()
add_executable(main main.cpp)
target_link_libraries(main PRIVATE Solver Graph Cluster)
target_compile_definitions(main PRIVATE RES_DIR="${CMAKE_SOURCE_DIR}/res/")