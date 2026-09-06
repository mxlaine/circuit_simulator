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
