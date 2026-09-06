#include <catch.hpp>
#include <filesystem>
#include <fstream>
#include "netlist_parser.hpp"
#include "simulator.hpp"

namespace {
struct NetlistFile {
  std::filesystem::path path = "parser_test_input.txt";
  explicit NetlistFile(const std::string& text) { std::ofstream(path) << text; }
  ~NetlistFile() { std::filesystem::remove(path); }
};
}
TEST_CASE("Parsed DC divider matches analytical voltage", "[parser]") {
  NetlistFile file("  # divider\n\n.DC\nV1 1 0 12\nR1 1 2 3000\nR2 2 0 1000\n");
  auto circuit = NetlistParser().ParseNetlist(file.path.string());
  Simulator(circuit.get()).SolveDC();
  REQUIRE(circuit->GetOrCreateNode("2")->voltage_ == Approx(3.0));
}
TEST_CASE("Parsed AC circuit retains frequency and inductance", "[parser]") {
  NetlistFile file(".AC 1000\nV1 1 0 1\nL1 1 2 0.001\nR1 2 0 1000\n");
  auto circuit = NetlistParser().ParseNetlist(file.path.string());
  REQUIRE(circuit->GetFrequency() == Approx(1000));
  REQUIRE(circuit->GetInductors().size() == 1);
  REQUIRE_NOTHROW(Simulator(circuit.get()).SolveAC(1000.0));
}
TEST_CASE("Malformed netlists fail instead of returning partial circuits", "[parser]") {
  auto text = GENERATE("", " # comment only\n", "R1 a GND 10\n", ".AC\n", ".DC\nR1 a GND\n", ".DC\nX1 a GND 1\n");
  NetlistFile file(text);
  REQUIRE_THROWS_AS(NetlistParser().ParseNetlist(file.path.string()), std::runtime_error);
}
TEST_CASE("Missing netlist file is rejected", "[parser]") {
  REQUIRE_THROWS_AS(NetlistParser().ParseNetlist("nonexistent-directory/input.net"), std::runtime_error);
}
