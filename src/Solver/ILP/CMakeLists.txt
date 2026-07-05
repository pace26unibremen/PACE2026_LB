# src/Solver/ILP/CMakeLists.txt
set(ilp_sources
    ILPModel.cpp
    TreeUtils.cpp
    IncrementalMAFSolver.cpp
    IncrUWrMaxSATSolver.cpp
)

target_sources(Solver PRIVATE ${ilp_sources})

target_include_directories(Solver PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/UWrMaxSat/uwrmaxsat/cadical/src
    ${CMAKE_SOURCE_DIR}/lib/UWrMaxSat/uwrmaxsat
    ${CMAKE_SOURCE_DIR}/lib/UWrMaxSat/uwrmaxsat/ADTs
    ${CMAKE_SOURCE_DIR}/lib/UWrMaxSat/cominisatps
    ${CMAKE_SOURCE_DIR}/lib/UWrMaxSat/cominisatps/minisat
)

target_compile_definitions(Solver PRIVATE
    CADICAL
    NDEBUG
    _FILE_OFFSET_BITS=64
    __STDC_FORMAT_MACROS
    __STDC_LIMIT_MACROS
)

target_compile_options(Solver PRIVATE
    "SHELL:-include ${CMAKE_SOURCE_DIR}/lib/UWrMaxSat/uwrmaxsat/ADTs/Global.h"
    "SHELL:-include ${CMAKE_SOURCE_DIR}/lib/UWrMaxSat/uwrmaxsat/Main.h"
)

target_link_libraries(Solver PRIVATE UWrMaxSat)