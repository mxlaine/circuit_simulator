cmake_minimum_required(VERSION 3.11)
project(circuit-simulator-1)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

# Enable testing globally so add_test() in subdirs works
include(CTest)
enable_testing()

message(STATUS "BUILD directory is ${CMAKE_BINARY_DIR}")
message(STATUS "SRC directory is ${CMAKE_SOURCE_DIR}")

# SOURCE AND HEADER FILES
file(GLOB_RECURSE SOURCES_HPP 
    "${CMAKE_SOURCE_DIR}/src/core/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/sim/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/io/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/gui/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/*.hpp"
)

file(GLOB_RECURSE SOURCES_H 
    "${CMAKE_SOURCE_DIR}/src/core/*.h"
    "${CMAKE_SOURCE_DIR}/src/sim/*.h"
    "${CMAKE_SOURCE_DIR}/src/io/*.h"
    "${CMAKE_SOURCE_DIR}/src/gui/*.h"
)

file(GLOB_RECURSE SOURCES_CPP 
    "${CMAKE_SOURCE_DIR}/src/core/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/sim/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/io/*.cpp"
)

# Add top-level sources explicitly to avoid unintentionally pulling GUI sources via recursive globs
set(TOPLEVEL_SOURCES
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
)

file(GLOB_RECURSE SOURCES_HPP 
    "${CMAKE_SOURCE_DIR}/src/core/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/sim/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/io/*.hpp"
    "${CMAKE_SOURCE_DIR}/src/*.hpp"
)

file(GLOB_RECURSE SOURCES_C 
    "${CMAKE_SOURCE_DIR}/src/core/*.c"
    "${CMAKE_SOURCE_DIR}/src/sim/*.c"
    "${CMAKE_SOURCE_DIR}/src/io/*.c"
)

file(GLOB_RECURSE SOURCES_H 
    "${CMAKE_SOURCE_DIR}/src/core/*.h"
    "${CMAKE_SOURCE_DIR}/src/sim/*.h"
    "${CMAKE_SOURCE_DIR}/src/io/*.h"
)

set(SOURCES ${SOURCES_C} ${SOURCES_CPP} ${SOURCES_H} ${SOURCES_HPP} ${TOPLEVEL_SOURCES})

message(STATUS "SRC directories source code files: 
\t-- HEADERS: ${SOURCES_H} ${SOURCES_HPP}
\t-- C SOURCES: ${SOURCES_C}
\t-- CPP SOURCES: ${SOURCES_CPP}
\t-- ALL: ${SOURCES}")

# --- Main executable ---
add_executable(circuit_sim)
target_sources(circuit_sim PUBLIC ${SOURCES})

target_include_directories(circuit_sim PUBLIC 
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/sim
    ${CMAKE_SOURCE_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/libs
)

if(MSVC)
    target_compile_options(circuit_sim PRIVATE /Wall)
else()
    target_compile_options(circuit_sim PRIVATE -Wall -Wextra -pedantic -Wno-missing-field-initializers)
endif()

# --- GUI (Qt) subproject ---
option(BUILD_GUI "Build Qt GUI from src/gui" ON)
if(BUILD_GUI)
    add_subdirectory(${CMAKE_SOURCE_DIR}/src/gui ${CMAKE_BINARY_DIR}/gui)
endif()

# --- Add test suite ---
add_subdirectory(tests)
