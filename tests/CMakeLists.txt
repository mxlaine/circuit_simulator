cmake_minimum_required(VERSION 3.11)
project(circuit_sim_tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

# Include Catch2 and Eigen (from libs)
include_directories(${CMAKE_SOURCE_DIR}/libs)

# Find Eigen if available system-wide (optional)
find_path(EIGEN3_INCLUDE_DIR Eigen/Dense)
if(EIGEN3_INCLUDE_DIR)
    include_directories(${EIGEN3_INCLUDE_DIR})
endif()

# Core simulator sources (so tests link to your logic)
file(GLOB_RECURSE CORE_SOURCES
    "${CMAKE_SOURCE_DIR}/src/core/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/sim/*.cpp"
)

# Test sources
file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Create the test executable
add_executable(sim_tests ${TEST_SOURCES} ${CORE_SOURCES})

# Include project headers
target_include_directories(sim_tests PUBLIC
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/sim
    ${CMAKE_SOURCE_DIR}/libs
)

# Enable warnings
if(MSVC)
    target_compile_options(sim_tests PRIVATE /Wall)
else()
    target_compile_options(sim_tests PRIVATE -Wall -Wextra -pedantic)
endif()

# --- Register Catch2 test with CTest ---
add_test(NAME simulator_tests COMMAND sim_tests)