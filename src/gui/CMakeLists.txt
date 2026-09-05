cmake_minimum_required(VERSION 3.16)

project(circuit_sim_gui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Had to add so it runs on Win11 + WSL
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.hpp
        mainwindow.ui
        gridwidget.cpp
        gridwidget.hpp
        component.hpp
        componentdialog.cpp
        componentdialog.hpp
        plotdialog.cpp
        plotdialog.hpp
        ${CMAKE_SOURCE_DIR}/libs/qcustomplot.cpp
        ${CMAKE_SOURCE_DIR}/libs/qcustomplot.h
        ../io/netlist_parser.cpp
        ../sim/simulator.cpp
        ../core/circuit.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(circuit_sim_gui
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )

else()
    add_executable(circuit_sim_gui
        ${PROJECT_SOURCES}
    )
endif()

target_link_libraries(circuit_sim_gui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

target_include_directories(circuit_sim_gui PRIVATE
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/io
    ${CMAKE_SOURCE_DIR}/src/sim
    ${CMAKE_SOURCE_DIR}/libs
)

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.circuit_sim_gui)
endif()
set_target_properties(circuit_sim_gui PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS circuit_sim_gui
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(circuit_sim_gui)
endif()
