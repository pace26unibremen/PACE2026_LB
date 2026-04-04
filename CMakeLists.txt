cmake_minimum_required(VERSION 3.27.0)

# This defining has been generated using LLM ->.
if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_C_COMPILER clang CACHE STRING "" FORCE)
    set(CMAKE_CXX_COMPILER clang++ CACHE STRING "" FORCE)
endif() # <- LLM, whereas project() definition has been changed to incorporate C CXX. Theoretically, if Clang is not
# installed it should default to GCC.

project(Pace2026 VERSION 0.1.0 LANGUAGES C CXX)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(WARNING "Clang++ is mandatory for Coverage/profiling builds ")
endif()


include(CMakePrintHelpers)

message(STATUS "CMake Version: ${CMAKE_VERSION}")

option(DEBUG_IMAGE_VIEW_GRAPH
       "Adds option to view graphs as image in debugger"
       OFF)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -Wall")

set(CMAKE_CXX_FLAGS_COVERAGE "${CMAKE_CXX_FLAGS_COVERAGE}  -fprofile-instr-generate -fcoverage-mapping")
set(CMAKE_CXX_FLAGS_PROFILING  "${CMAKE_CXX_FLAGS_PROFILING}  -O2 -g -gline-tables-only -fdebug-info-for-profiling -funique-internal-linkage-names")

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(tests)

