add_executable(sim_tests
    test_simulator.cpp test_dc.cpp test_ac.cpp test_dependent_sources.cpp
    test_opamp.cpp test_topology.cpp test_netlist_parser.cpp
)
target_link_libraries(sim_tests PRIVATE circuit_sim_core)
add_test(NAME simulator_tests COMMAND sim_tests)
