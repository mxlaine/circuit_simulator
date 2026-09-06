#include <catch.hpp>
#include <complex>
#include <vector>

#include "capacitor.hpp"
#include "circuit.hpp"
#include "component.hpp"
#include "current_source.hpp"
#include "inductor.hpp"
#include "node.hpp"
#include "resistor.hpp"
#include "simulator.hpp"
#include "voltage_source.hpp"

TEST_CASE("Node extraction and indexing", "[circuit][nodes]") {
  Node gnd(0, "GND");
  Node n1(1), n2(2);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddResistor("R1", 1000.0, &n1, &n2);
  circuit->AddResistor("R2", 2000.0, &n2, &gnd);
  circuit->extractNodes();

  auto nodes = circuit->GetNodes();
  REQUIRE(nodes.size() == 2);  // n1 and n2 (GND excluded)
}

TEST_CASE("GetOrCreateNode functionality", "[circuit][nodes]") {
  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();

  Node* n1 = circuit->GetOrCreateNode("1");
  Node* n1_again = circuit->GetOrCreateNode("1");
  Node* n2 = circuit->GetOrCreateNode("2");

  REQUIRE(n1 == n1_again);  // Should return same node
  REQUIRE(n1 != n2);
  REQUIRE(n1->GetName() == "1");
  REQUIRE(n2->GetName() == "2");
}

TEST_CASE("Zero resistance handling", "[sim][edge]") {
  Node gnd(0, "GND");
  Node n1(1);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("V1", 10.0, &n1, &gnd);
  // Very small resistance to avoid division by zero
  circuit->AddResistor("R1", 0.001, &n1, &gnd);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  REQUIRE_NOTHROW(sim.SolveDC());
}

TEST_CASE("High resistance values", "[sim][edge]") {
  Node gnd(0, "GND");
  Node n1(1);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("V1", 5.0, &n1, &gnd);
  circuit->AddResistor("R1", 1e9, &n1, &gnd);  // 1GΩ
  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveDC();

  REQUIRE(n1.voltage_ == Approx(5.0));
  REQUIRE(circuit->GetResistors()[0]->current_ == Approx(5e-9).margin(1e-12));
}

TEST_CASE("Circuit without ground is rejected in DC and AC", "[sim][topology]") {
  Node n1(1), n2(2);
  Circuit circuit;
  circuit.AddVoltageSource("V1", 1.0, &n1, &n2);
  circuit.AddResistor("R1", 1000.0, &n1, &n2);
  Simulator sim(&circuit);
  REQUIRE_THROWS_WITH(sim.SolveDC(), "Circuit has no ground reference. Add a GND node.");
  REQUIRE_THROWS_WITH(sim.SolveAC(1000.0), "Circuit has no ground reference. Add a GND node.");
}
