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

TEST_CASE("Simple voltage divider", "[sim][DC]") {
  Node gnd(0, "GND");
  Node n1(1);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("V1", 10.0, &n1, &gnd);
  circuit->AddResistor("R1", 1000.0, &n1, &gnd);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveDC();

  REQUIRE(n1.voltage_ == Approx(10.0));
  REQUIRE(circuit->GetResistors()[0]->current_ == Approx(0.01));
}

TEST_CASE("Two resistor voltage divider", "[sim][DC]") {
  Node gnd(0, "GND");
  Node n1(1);
  Node n2(2);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("V1", 12.0, &n1, &gnd);
  circuit->AddResistor("R1", 3000.0, &n1, &n2);
  circuit->AddResistor("R2", 1000.0, &n2, &gnd);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveDC();

  REQUIRE(n1.voltage_ == Approx(12.0));
  REQUIRE(n2.voltage_ == Approx(3.0));  // Vout = Vin * R2/(R1+R2)
}

TEST_CASE("Parallel resistors", "[sim][DC]") {
  Node gnd(0, "GND");
  Node n1(1);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("V1", 5.0, &n1, &gnd);
  circuit->AddResistor("R1", 1000.0, &n1, &gnd);
  circuit->AddResistor("R2", 1000.0, &n1, &gnd);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveDC();

  REQUIRE(n1.voltage_ == Approx(5.0));
  // Total current = V / Req, Req = R1||R2 = 500Ω
  double total_current = circuit->GetResistors()[0]->current_ +
                         circuit->GetResistors()[1]->current_;
  REQUIRE(total_current == Approx(0.01));
}

TEST_CASE("Wheatstone bridge balanced", "[sim][DC]") {
  Node gnd(0, "GND");
  Node n1(1), n2(2), n3(3);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("V1", 10.0, &n1, &gnd);
  circuit->AddResistor("R1", 1000.0, &n1, &n2);
  circuit->AddResistor("R2", 1000.0, &n1, &n3);
  circuit->AddResistor("R3", 1000.0, &n2, &gnd);
  circuit->AddResistor("R4", 1000.0, &n3, &gnd);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveDC();

  // Bridge is balanced, n2 and n3 should have same voltage
  REQUIRE(n2.voltage_ == Approx(n3.voltage_));
  REQUIRE(n2.voltage_ == Approx(5.0));
}

TEST_CASE("Single current source", "[sim][DC][current]") {
  Node gnd(0, "GND");
  Node n1(1);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddCurrentSource("I1", 0.001, &gnd, &n1);  // 1mA
  circuit->AddResistor("R1", 1000.0, &n1, &gnd);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveDC();

  REQUIRE(n1.voltage_ == Approx(1.0));  // V = I*R = 0.001*1000
}

TEST_CASE("Current source with two resistors", "[sim][DC][current]") {
  Node gnd(0, "GND");
  Node n1(1), n2(2);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddCurrentSource("I1", 0.002, &gnd, &n1);  // 2mA
  circuit->AddResistor("R1", 1000.0, &n1, &n2);
  circuit->AddResistor("R2", 2000.0, &n2, &gnd);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveDC();

  REQUIRE(n1.voltage_ == Approx(6.0));
  REQUIRE(n2.voltage_ == Approx(4.0));
}

TEST_CASE("Mixed voltage and current sources", "[sim][DC]") {
  Node gnd(0, "GND");
  Node n1(1), n2(2);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("V1", 5.0, &n1, &gnd);
  circuit->AddCurrentSource("I1", 0.001, &gnd, &n2);
  circuit->AddResistor("R1", 1000.0, &n1, &n2);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveDC();

  REQUIRE(n1.voltage_ == Approx(5.0));
  REQUIRE(n2.voltage_ == Approx(6.0));
}

TEST_CASE("Cascaded voltage dividers", "[sim][DC]") {
  Node gnd(0, "GND");
  Node n1(1), n2(2), n3(3);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("V1", 12.0, &n1, &gnd);
  circuit->AddResistor("R1", 1000.0, &n1, &n2);
  circuit->AddResistor("R2", 1000.0, &n2, &gnd);
  circuit->AddResistor("R3", 2000.0, &n2, &n3);
  circuit->AddResistor("R4", 2000.0, &n3, &gnd);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveDC();

  REQUIRE(n1.voltage_ == Approx(12.0));
  // n2: voltage divider with loading from R3||R4
  // R2 || (R3 + R4) = 1000 || 4000 = 800Ω
  // v2 = 12 * 800 / (1000 + 800) = 5.333V
  REQUIRE(n2.voltage_ == Approx(5.333).margin(0.01));
  // n3: divider from n2
  // v3 = v2 * R4/(R3+R4) = 5.333 * 0.5 = 2.667V
  REQUIRE(n3.voltage_ == Approx(2.667).margin(0.01));
}

TEST_CASE("Multiple voltage sources", "[sim][DC]") {
  Node gnd(0, "GND");
  Node n1(1), n2(2), n3(3);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("V1", 5.0, &n1, &gnd);
  circuit->AddVoltageSource("V2", 3.0, &n2, &gnd);
  circuit->AddResistor("R1", 1000.0, &n1, &n3);
  circuit->AddResistor("R2", 1000.0, &n2, &n3);
  circuit->AddResistor("R3", 1000.0, &n3, &gnd);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveDC();

  // Superposition: n3 voltage from both sources
  REQUIRE(n3.voltage_ == Approx(8.0 / 3.0));
}

TEST_CASE("Multiple parallel paths", "[sim][DC]") {
  Node gnd(0, "GND");
  Node n1(1), n2(2);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("V1", 10.0, &n1, &gnd);

  // Three parallel paths between n1 and n2
  circuit->AddResistor("R1", 1000.0, &n1, &n2);
  circuit->AddResistor("R2", 1000.0, &n1, &n2);
  circuit->AddResistor("R3", 1000.0, &n1, &n2);
  circuit->AddResistor("R4", 1000.0, &n2, &gnd);

  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveDC();

  REQUIRE(n1.voltage_ == Approx(10.0));
  REQUIRE(n2.voltage_ == Approx(7.5).margin(0.01));
}
