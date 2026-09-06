cmake_minimum_required(VERSION 3.16)
project(circuit_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CTest)

add_library(circuit_sim_core
    src/core/circuit.cpp
    src/sim/simulator.cpp
    src/io/netlist_parser.cpp
)
target_include_directories(circuit_sim_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/sim
    ${CMAKE_CURRENT_SOURCE_DIR}/src/io
    ${CMAKE_CURRENT_SOURCE_DIR}/libs
)
if(MSVC)
    target_compile_options(circuit_sim_core PRIVATE /W4)
else()
    target_compile_options(circuit_sim_core PRIVATE -Wall -Wextra -pedantic)
endif()

add_executable(circuit_sim src/main.cpp)
target_link_libraries(circuit_sim PRIVATE circuit_sim_core)

option(BUILD_GUI "Build Qt GUI" ON)
if(BUILD_GUI)
    add_subdirectory(src/gui gui)
endif()
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

add_executable(butterworth_demo examples/butterworth.cpp)
target_link_libraries(butterworth_demo PRIVATE circuit_sim_core)
if(BUILD_TESTING)
    add_test(NAME butterworth_validation COMMAND butterworth_demo ${CMAKE_CURRENT_BINARY_DIR}/butterworth.csv)
endif()
