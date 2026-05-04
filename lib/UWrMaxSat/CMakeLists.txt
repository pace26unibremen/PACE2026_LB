cmake_minimum_required(VERSION 3.12)
message(STATUS "Configuring UWrMaxSat")

# ── Pfade ─────────────────────────────────────────────────────────────────────
set(UWR_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/uwrmaxsat)
set(CADICAL_DIR ${UWR_DIR}/cadical)

# ── Quellen ───────────────────────────────────────────────────────────────────
set(UWRMAXSAT_SOURCES
    ${UWR_DIR}/ADTs/FEnv.cc
    ${UWR_DIR}/ADTs/File.cc
    ${UWR_DIR}/ADTs/Global.cc
    ${UWR_DIR}/Debug.cc
    ${UWR_DIR}/Hardware_adders.cc
    ${UWR_DIR}/Hardware_clausify.cc
    ${UWR_DIR}/Hardware_sorters.cc
    ${UWR_DIR}/Hardware_reuse_sorters.cc
    ${UWR_DIR}/PbParser.cc
    ${UWR_DIR}/PbSolver.cc
    ${UWR_DIR}/PbSolver_convert.cc
    ${UWR_DIR}/PbSolver_convertAdd.cc
    ${UWR_DIR}/PbSolver_convertBdd.cc
    ${UWR_DIR}/PbSolver_convertSort.cc
    ${UWR_DIR}/MsSolver.cc
    ${UWR_DIR}/SatSolver.cc
    ${UWR_DIR}/System.cc
    ${UWR_DIR}/Main_utils.cc
    ${UWR_DIR}/Pre_separator.cc
    ${UWR_DIR}/ipamir.cc
)

# ── CaDiCaL bauen (falls libcadical.a noch nicht existiert) ───────────────────
set(CADICAL_LIB_PATH ${CADICAL_DIR}/build/libcadical.a)

if(NOT EXISTS ${CADICAL_LIB_PATH})
    message(STATUS "libcadical.a nicht gefunden - baue CaDiCaL...")
    execute_process(
        COMMAND sh -c "cd ${CADICAL_DIR} && echo 'cd worked'"
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE
    )
   execute_process(
        COMMAND sh -c "cd ${CADICAL_DIR} && CXX=/usr/bin/clang++ ./configure --no-contracts --no-tracing && make cadical 2>&1; echo 'exit: '$?"
        RESULT_VARIABLE CADICAL_BUILD_RESULT
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE
    )

    if(NOT CADICAL_BUILD_RESULT EQUAL 0)
        message(STATUS "Output: ${CADICAL_BUILD_OUTPUT}")
        message(STATUS "Error:  ${CADICAL_BUILD_ERROR}")
        message(FATAL_ERROR "CaDiCaL Build fehlgeschlagen!")
    endif()
else()
    message(STATUS "CaDiCaL gefunden: ${CADICAL_LIB_PATH}")
endif()

# ── Imported Target für CaDiCaL ───────────────────────────────────────────────
add_library(cadical_lib STATIC IMPORTED)
set_target_properties(cadical_lib PROPERTIES
    IMPORTED_LOCATION ${CADICAL_LIB_PATH}
)

# ── Statische Library ─────────────────────────────────────────────────────────
add_library(UWrMaxSat STATIC ${UWRMAXSAT_SOURCES})

# ── C++ Standard ──────────────────────────────────────────────────────────────
target_compile_features(UWrMaxSat PUBLIC cxx_std_17)

# ── Includes ──────────────────────────────────────────────────────────────────
target_include_directories(UWrMaxSat
    PUBLIC
        ${UWR_DIR}
        ${UWR_DIR}/ADTs
    PRIVATE
        ${CADICAL_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/cominisatps
)

# ── Compile Definitions (aus config.cadical) ──────────────────────────────────
target_compile_definitions(UWrMaxSat
    PRIVATE
        CADICAL
        NDEBUG
        _FILE_OFFSET_BITS=64
        __STDC_FORMAT_MACROS
        __STDC_LIMIT_MACROS
)

# ── Forced Includes (Original: -include Global.h -include Main.h) ─────────────
target_compile_options(UWrMaxSat PRIVATE
    -Wno-strict-aliasing
    "SHELL:-include ${UWR_DIR}/ADTs/Global.h"
    "SHELL:-include ${UWR_DIR}/Main.h"
)

# ── GMP ───────────────────────────────────────────────────────────────────────
find_library(GMP_LIBRARY NAMES gmp REQUIRED)
message(STATUS "GMP gefunden: ${GMP_LIBRARY}")

# ── Linken ────────────────────────────────────────────────────────────────────
target_link_libraries(UWrMaxSat
    PUBLIC
        cadical_lib
        ${GMP_LIBRARY}
)