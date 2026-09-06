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

TEST_CASE("VCVS amplifier", "[sim][VCVS]") {
  Node gnd(0, "GND");
  Node n1(1), n2(2), n3(3);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("Vin", 1.0, &n1, &gnd);
  circuit->AddResistor("R1", 1000.0, &n1, &n2);
  circuit->AddVCVS("E1", 10.0, &n3, &gnd, &n2, &gnd);  // Gain of 10
  circuit->AddResistor("RL", 1000.0, &n3, &gnd);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveDC();

  auto nodes = circuit->GetNodes();
  REQUIRE(nodes[2]->voltage_ == Approx(10.0 * nodes[1]->voltage_).margin(0.1));
}

TEST_CASE("VCVS A matrix build", "[sim][VCVS]") {
  // Create nodes
  Node gnd(0, "GND");
  Node n1(1);
  Node n2(2);
  Node n3(3);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();

  // Create resistors
  circuit->AddResistor("R1", 1 / 1000.0, &gnd, &n1);
  circuit->AddResistor("R2", 1 / 2000.0, &n1, &n2);
  circuit->AddResistor("R3", 1 / 3000.0, &n2, &n3);
  circuit->AddResistor("R4", 1 / 5000.0, &n3, &gnd);

  circuit->extractNodes();

  circuit->AddVCVS("A(v2 - v3)", 91.0, &n2, &n1, &n2, &n3);
  auto resistors = circuit->GetResistors();
  auto VCVS = circuit->GetVCVSs()[0];

  // Prepare simulator
  Simulator sim(circuit.get());

  SECTION("Matrix build") {
    Eigen::MatrixXd A;
    sim.FillAMatrix(A);
    REQUIRE(A.rows() == 4);
    REQUIRE(A.cols() == 4);

    SECTION("Matrix values match expected conductances") {
      REQUIRE(A(0, 0) ==
              Approx(resistors[0]->GetValue() + resistors[1]->GetValue()));
      REQUIRE(A(0, 1) == Approx(resistors[1]->GetValue() * -1.0));
      REQUIRE(A(1, 0) == Approx(resistors[1]->GetValue() * -1.0));
      REQUIRE(A(1, 1) ==
              Approx(resistors[2]->GetValue() + resistors[1]->GetValue()));
      REQUIRE(A(1, 2) == Approx(resistors[2]->GetValue() * -1.0));
      REQUIRE(A(2, 1) == Approx(resistors[2]->GetValue() * -1.0));
      REQUIRE(A(2, 2) ==
              Approx(resistors[2]->GetValue() + resistors[3]->GetValue()));
      REQUIRE(A(3, 0) == Approx(-1.0));
      REQUIRE(A(0, 3) == Approx(-1.0));
      REQUIRE(A(3, 1) == Approx(1.0 - VCVS->GetValue()));
      REQUIRE(A(1, 3) == Approx(1.0));
      REQUIRE(A(3, 2) == Approx(VCVS->GetValue()));
    }
  }
}

TEST_CASE("VCCS transconductance amplifier", "[sim][VCCS]") {
  Node gnd(0, "GND");
  Node n1(1), n2(2);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("Vin", 1.0, &n1, &gnd);
  circuit->AddResistor("Rin", 1000.0, &n1, &gnd);
  circuit->AddVCCS("G1", 0.01, &n2, &gnd, &n1,
                   &gnd);  // gm = 10mS, current flows from n2 to gnd
  circuit->AddResistor("RL", 1000.0, &n2, &gnd);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveDC();

  auto nodes = circuit->GetNodes();
  // VCCS injects current proportional to input voltage
  // Current direction: from n2 to gnd creates negative voltage at n2
  // Iout = gm * Vin = 0.01 * 1.0 = 0.01 A
  // Vout = -Iout * RL (negative because current flows to ground)
  REQUIRE(std::abs(nodes[1]->voltage_) == Approx(10.0).margin(0.1));
}

TEST_CASE("VCCS A Matrix build", "[sim][VCCS]") {
  // Create nodes
  Node gnd(0, "GND");
  Node n1(1);
  Node n2(2);
  Node n3(3);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();

  // Create resistors
  circuit->AddResistor("R1", 1 / 1000.0, &gnd, &n1);
  circuit->AddResistor("R2", 1 / 2000.0, &n2, &n3);
  circuit->AddVCCS("gm(v2 - v3)", 98, &n2, &n1, &n2, &n3);

  circuit->extractNodes();

  auto resistors = circuit->GetResistors();
  auto VCCS = circuit->GetVCCSs()[0];

  // Prepare simulator
  Simulator sim(circuit.get());

  SECTION("Matrix build") {
    Eigen::MatrixXd A;
    sim.FillAMatrix(A);
    REQUIRE(A.rows() == 3);
    REQUIRE(A.cols() == 3);

    SECTION("Matrix values match expected conductances") {
      REQUIRE(A(0, 0) == Approx(resistors[0]->GetValue()));
      REQUIRE(A(0, 1) == Approx(VCCS->GetValue()));
      REQUIRE(A(0, 2) == Approx(-1.0 * VCCS->GetValue()));
      REQUIRE(A(1, 1) == Approx(resistors[1]->GetValue() - VCCS->GetValue()));
      REQUIRE(A(1, 2) == Approx(VCCS->GetValue() - resistors[1]->GetValue()));
      REQUIRE(A(2, 1) == Approx(resistors[1]->GetValue() * -1.0));
      REQUIRE(A(2, 2) == Approx(resistors[1]->GetValue()));
    }
  }
}

TEST_CASE("CCCS current mirror", "[sim][CCCS]") {
  Node gnd(0, "GND");
  Node n1(1), n2(2), n3(3);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("V1", 10.0, &n1, &gnd);
  auto sc = circuit->AddShortCircuit("SC1", &n1, &n2);
  circuit->AddResistor("R1", 1000.0, &n2, &gnd);
  circuit->AddCCCS("F1", 2.0, &gnd, &n3, sc);  // Current gain of 2
  circuit->AddResistor("R2", 500.0, &n3, &gnd);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveDC();

  auto cccs = circuit->GetCCCSs()[0];
  double ref_current = sc->current_;
  REQUIRE(cccs->current_ == Approx(2.0 * ref_current).margin(0.01));
}

TEST_CASE("CCCS A matrix build", "[sim][CCCS]") {
  // Create nodes
  Node gnd(0, "GND");
  Node n1(1);
  Node n2(2);
  Node n3(3);
  Node n4(4);
  Node n5(5);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();

  // Create resistors
  circuit->AddResistor("gs", 1 / 1000.0, &n1, &n2);
  circuit->AddResistor("ge", 1 / 2000.0, &n2, &n4);
  circuit->AddResistor("gE", 1 / 5000.0, &n5, &gnd);
  circuit->AddResistor("gC", 1 / 4000.0, &n3, &gnd);
  circuit->AddShortCircuit("SC", &n4, &n5);
  circuit->AddVoltageSource("V1", 200, &n1, &gnd);
  circuit->AddCCCS(
      "Aie", 73, &n2, &n3,
      dynamic_cast<ShortCircuit*>(circuit->GetVoltageSources()[0]));

  circuit->extractNodes();

  auto resistors = circuit->GetResistors();
  auto CCCS = circuit->GetCCCSs()[0];

  // Prepare simulator
  Simulator sim(circuit.get());

  SECTION("Matrix build") {
    Eigen::MatrixXd A;
    sim.FillAMatrix(A);
    REQUIRE(A.rows() == 7);
    REQUIRE(A.cols() == 7);

    SECTION("Matrix values match expected conductances") {
      REQUIRE(A(0, 0) == Approx(resistors[0]->GetValue()));
      REQUIRE(A(0, 1) == Approx(-1.0 * resistors[0]->GetValue()));
      REQUIRE(A(0, 6) == Approx(1.0));

      REQUIRE(A(1, 0) == Approx(-1.0 * resistors[0]->GetValue()));
      REQUIRE(A(1, 1) ==
              Approx(resistors[0]->GetValue() + resistors[1]->GetValue()));
      REQUIRE(A(1, 3) == Approx(-1.0 * resistors[1]->GetValue()));
      REQUIRE(A(1, 5) == Approx(-1.0 * CCCS->GetValue()));

      REQUIRE(A(2, 2) == Approx(resistors[3]->GetValue()));
      REQUIRE(A(2, 5) == Approx(CCCS->GetValue()));

      REQUIRE(A(3, 1) == Approx(-1.0 * resistors[1]->GetValue()));
      REQUIRE(A(3, 3) == Approx(resistors[1]->GetValue()));
      REQUIRE(A(3, 5) == Approx(1.0));

      REQUIRE(A(4, 4) == Approx(resistors[2]->GetValue()));
      REQUIRE(A(4, 5) == Approx(-1.0));

      REQUIRE(A(5, 3) == Approx(1.0));
      REQUIRE(A(5, 4) == Approx(-1.0));

      REQUIRE(A(6, 0) == Approx(1.0));
    }
  }
}

TEST_CASE("CCVS transresistance amplifier", "[sim][CCVS]") {
  Node gnd(0, "GND");
  Node n1(1), n2(2), n3(3);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("V1", 5.0, &n1, &gnd);
  auto sc = circuit->AddShortCircuit("SC1", &n1, &n2);
  circuit->AddResistor("R1", 1000.0, &n2, &gnd);
  circuit->AddCCVS("H1", 1000.0, &n3, &gnd, sc);  // Rm = 1kΩ
  circuit->AddResistor("RL", 1000.0, &n3, &gnd);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveDC();

  auto nodes = circuit->GetNodes();
  double ref_current = sc->current_;
  REQUIRE(nodes[2]->voltage_ == Approx(1000.0 * ref_current).margin(0.1));
}

TEST_CASE("CCVS A matrix build", "[sim][CCVS]") {
  // Create nodes
  Node gnd(0, "GND");
  Node n1(1);
  Node n2(2);
  Node n3(3);
  Node n4(4);
  Node n5(5);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();

  // Create resistors
  circuit->AddVoltageSource("V1", 200, &n1, &gnd);
  circuit->AddResistor("gs", 1 / 1000.0, &n1, &n2);
  circuit->AddShortCircuit("SC", &n2, &n3);
  circuit->AddResistor("gi", 1 / 2000.0, &n3, &gnd);
  circuit->AddResistor("go", 1 / 4000.0, &n4, &n5);
  circuit->AddResistor("gL", 1 / 4000.0, &n5, &gnd);
  circuit->AddCCVS(
      "Rmii", 73, &n4, &gnd,
      dynamic_cast<ShortCircuit*>(circuit->GetVoltageSources()[1]));

  circuit->extractNodes();

  auto resistors = circuit->GetResistors();
  auto CCVS = circuit->GetCCVSs()[0];

  // Prepare simulator
  Simulator sim(circuit.get());

  SECTION("Matrix build") {
    Eigen::MatrixXd A;
    sim.FillAMatrix(A);
    REQUIRE(A.rows() == 8);
    REQUIRE(A.cols() == 8);

    SECTION("Matrix values match expected conductances") {
      REQUIRE(A(0, 0) == Approx(resistors[0]->GetValue()));
      REQUIRE(A(0, 1) == Approx(-1.0 * resistors[0]->GetValue()));
      REQUIRE(A(0, 5) == Approx(1.0));

      REQUIRE(A(1, 0) == Approx(-1.0 * resistors[0]->GetValue()));
      REQUIRE(A(1, 1) == Approx(resistors[0]->GetValue()));
      REQUIRE(A(1, 6) == Approx(1.0));

      REQUIRE(A(2, 2) == Approx(resistors[1]->GetValue()));
      REQUIRE(A(2, 6) == Approx(-1.0));

      REQUIRE(A(3, 3) == Approx(resistors[2]->GetValue()));
      REQUIRE(A(3, 4) == Approx(-1.0 * resistors[2]->GetValue()));
      REQUIRE(A(3, 7) == Approx(1.0));

      REQUIRE(A(4, 3) == Approx(-1.0 * resistors[2]->GetValue()));
      REQUIRE(A(4, 4) ==
              Approx(resistors[2]->GetValue() + resistors[3]->GetValue()));

      REQUIRE(A(5, 0) == Approx(1.0));

      REQUIRE(A(6, 1) == Approx(1.0));
      REQUIRE(A(6, 2) == Approx(-1.0));

      REQUIRE(A(7, 3) == Approx(1.0));
      REQUIRE(A(7, 6) == Approx(-1.0 * CCVS->GetValue()));
    }
  }
}
