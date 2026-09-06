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

TEST_CASE("Op-amp inverting amplifier", "[sim][OpAmp]") {
  Node gnd(0, "GND");
  Node n1(1), n2(2), n3(3);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("Vin", 1.0, &n1, &gnd);
  circuit->AddResistor("R1", 1000.0, &n1, &n2);
  circuit->AddResistor("Rf", 10000.0, &n2, &n3);
  circuit->AddOpAmp("OA1", &gnd, &n2, &n3);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveDC();

  auto nodes = circuit->GetNodes();
  // Gain = -Rf/R1 = -10
  REQUIRE(nodes[2]->voltage_ == Approx(-10.0).margin(0.1));
}

TEST_CASE("Op-amp non-inverting amplifier", "[sim][OpAmp]") {
  Node gnd(0, "GND");
  Node n1(1), n2(2), n3(3);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("Vin", 1.0, &n1, &gnd);
  circuit->AddOpAmp("OA1", &n1, &n2, &n3);
  circuit->AddResistor("R1", 1000.0, &n2, &gnd);
  circuit->AddResistor("Rf", 9000.0, &n2, &n3);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveDC();

  auto nodes = circuit->GetNodes();
  // Gain = 1 + Rf/R1 = 10
  REQUIRE(nodes[2]->voltage_ == Approx(10.0).margin(0.1));
}

TEST_CASE("Op-amp voltage follower", "[sim][OpAmp]") {
  Node gnd(0, "GND");
  Node n1(1), n2(2);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("Vin", 5.0, &n1, &gnd);
  circuit->AddOpAmp("OA1", &n1, &n2, &n2);  // Output connected to inv input
  circuit->AddResistor("RL", 1000.0, &n2, &gnd);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveDC();

  auto nodes = circuit->GetNodes();
  REQUIRE(nodes[1]->voltage_ == Approx(5.0).margin(0.01));
}

TEST_CASE("OpAmp A matrix build", "[sim][OpAmp]") {
  // Create nodes
  Node gnd(0, "GND");
  Node n1(1);
  Node n2(2);
  Node n3(3);
  Node n4(4);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();

  // Create components
  circuit->AddVoltageSource("V1", 200, &n1, &gnd);
  circuit->AddResistor("R1", 1 / 1000.0, &n1, &n2);
  circuit->AddResistor("gi", 1 / 2000.0, &n3, &gnd);
  circuit->AddResistor("go", 1 / 4000.0, &n4, &n3);
  circuit->AddOpAmp("O1", &n2, &n3, &n4);

  circuit->extractNodes();

  auto resistors = circuit->GetResistors();

  // Prepare simulator
  Simulator sim(circuit.get());

  SECTION("Matrix build") {
    Eigen::MatrixXd A;
    sim.FillAMatrix(A);
    REQUIRE(A.rows() == 6);
    REQUIRE(A.cols() == 6);

    SECTION("Matrix values match expected conductances") {
      REQUIRE(A(0, 0) == Approx(resistors[0]->GetValue()));
      REQUIRE(A(0, 1) == Approx(-1.0 * resistors[0]->GetValue()));
      REQUIRE(A(0, 4) == Approx(1.0));

      REQUIRE(A(1, 0) == Approx(-1.0 * resistors[0]->GetValue()));
      REQUIRE(A(1, 1) == Approx(resistors[0]->GetValue()));

      REQUIRE(A(2, 2) ==
              Approx(resistors[1]->GetValue() + resistors[2]->GetValue()));
      REQUIRE(A(2, 3) == Approx(-1.0 * resistors[2]->GetValue()));

      REQUIRE(A(3, 2) == Approx(-1.0 * resistors[2]->GetValue()));
      REQUIRE(A(3, 3) == Approx(resistors[2]->GetValue()));
      REQUIRE(A(3, 5) == Approx(1.0));

      REQUIRE(A(4, 0) == Approx(1.0));

      REQUIRE(A(5, 1) == Approx(1.0));
      REQUIRE(A(5, 2) == Approx(-1.0));
    }
  }
}
