#ifndef CATCH_CONFIG_MAIN
#define CATCH_CONFIG_MAIN
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

// ============================================================
// BASIC DC TESTS
// ============================================================

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

// ============================================================
// CURRENT SOURCE TESTS
// ============================================================

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

// ============================================================
// AC ANALYSIS TESTS
// ============================================================

TEST_CASE("Pure resistive AC circuit", "[sim][AC]") {
  Node gnd(0, "GND");
  Node n1(1);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("V1", 10.0, &n1, &gnd);
  circuit->AddResistor("R1", 100.0, &n1, &gnd);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  sim.SolveAC(1000.0);

  REQUIRE(std::abs(n1.voltage_ac_) == Approx(10.0));
  REQUIRE(std::arg(n1.voltage_ac_) == Approx(0.0).margin(1e-6));
  REQUIRE(std::abs(circuit->GetResistors()[0]->complex_current_) ==
          Approx(0.1));
}

TEST_CASE("RC low-pass filter", "[sim][AC]") {
  Node gnd(0, "GND");
  Node n1(1), n2(2);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("V1", 1.0, &n1, &gnd);
  circuit->AddResistor("R1", 1000.0, &n1, &n2);
  circuit->AddCapacitor("C1", 1e-6, &n2, &gnd);  // 1µF
  circuit->extractNodes();

  Simulator sim(circuit.get());

  SECTION("At cutoff frequency") {
    double fc = 1.0 / (2.0 * M_PI * 1000.0 * 1e-6);  // ~159.15 Hz
    sim.SolveAC(fc);

    auto nodes = circuit->GetNodes();
    double gain =
        std::abs(nodes[1]->voltage_ac_) / std::abs(nodes[0]->voltage_ac_);
    REQUIRE(gain == Approx(0.7071).margin(0.01));  // -3dB point
  }
}

TEST_CASE("RL low-pass filter", "[sim][AC]") {
  Node gnd(0, "GND");
  Node n1(1), n2(2);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("V1", 1.0, &n1, &gnd);
  circuit->AddResistor("R1", 100.0, &n2, &gnd);
  circuit->AddInductor("L1", 1e-3, &n2, &n1);  // 1mH
  circuit->extractNodes();

  Simulator sim(circuit.get());

  SECTION("Low frequency pass") {
    sim.SolveAC(100.0);
    auto nodes = circuit->GetNodes();
    double gain = std::abs(nodes[1]->voltage_ac_);
    REQUIRE(gain > 0.9);  // Should pass most of signal at low freq
  }

  SECTION("High frequency attenuation") {
    sim.SolveAC(100000.0);
    auto nodes = circuit->GetNodes();
    double gain = std::abs(nodes[1]->voltage_ac_);
    REQUIRE(gain < 0.2);  // Should be attenuated at high freq
  }
}

TEST_CASE("LC resonant circuit", "[sim][AC]") {
  Node gnd(0, "GND");
  Node n1(1);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("V1", 1.0, &n1, &gnd);
  circuit->AddInductor("L1", 1e-3, &n1, &gnd);   // 1mH
  circuit->AddCapacitor("C1", 1e-6, &n1, &gnd);  // 1µF
  circuit->AddResistor("R1", 10.0, &n1, &gnd);   // Small resistance
  circuit->extractNodes();

  Simulator sim(circuit.get());

  // center freq f0
  double f0 = 1.0 / (2.0 * M_PI * std::sqrt(1e-3 * 1e-6));

  SECTION("At resonance") {
    sim.SolveAC(f0);
    auto inductor = circuit->GetInductors()[0];
    auto capacitor = circuit->GetCapacitors()[0];

    // At center frequency, inductor and capacitor currents should be equal
    // magnitude
    REQUIRE(std::abs(inductor->complex_current_) ==
            Approx(std::abs(capacitor->complex_current_)).margin(0.01));
  }
}

// ============================================================
// DEPENDENT SOURCE TESTS
// ============================================================

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

// ============================================================
// OP-AMP TESTS
// ============================================================

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

// ============================================================
// COMPLEX TOPOLOGY TESTS
// ============================================================

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
  REQUIRE(n3.voltage_ > 0.0);
  REQUIRE(n3.voltage_ < 5.0);
}

// ============================================================
// NODE MANAGEMENT TESTS
// ============================================================

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

// ============================================================
// EDGE CASES AND ERROR HANDLING
// ============================================================

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

TEST_CASE("Very high frequency AC", "[sim][AC][edge]") {
  Node gnd(0, "GND");
  Node n1(1), n2(2);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();
  circuit->AddVoltageSource("V1", 1.0, &n1, &gnd);
  circuit->AddCapacitor("C1", 1e-12, &n1, &n2);  // 1pF
  circuit->AddResistor("R1", 50.0, &n2, &gnd);
  circuit->extractNodes();

  Simulator sim(circuit.get());
  REQUIRE_NOTHROW(sim.SolveAC(1e9));  // 1 GHz
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

#endif