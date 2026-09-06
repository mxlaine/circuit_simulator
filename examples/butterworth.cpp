// Reproduce the magnitude response of saved_circuits/butterworth_5th_order_1khz.txt.
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "circuit.hpp"
#include "simulator.hpp"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: butterworth_demo output.csv\n";
    return 1;
  }
  std::ofstream output(argv[1]);
  if (!output) return 1;
  Node ground(0, "GND"), input(1), n1(2), n2(3), n3(4), out(5);
  Circuit circuit;
  circuit.AddVoltageSource("Vs", 1.0, &input, &ground);
  circuit.AddResistor("Rsrc", 1000.0, &input, &n1);
  circuit.AddInductor("L1", 0.0983631643, &n1, &n2);
  circuit.AddCapacitor("C2", 2.575181074e-7, &n2, &ground);
  circuit.AddInductor("L3", 0.3183098862, &n2, &n3);
  circuit.AddCapacitor("C4", 2.575181074e-7, &n3, &ground);
  circuit.AddInductor("L5", 0.0983631643, &n3, &out);
  circuit.AddResistor("Rload", 1000.0, &out, &ground);
  Simulator simulator(&circuit);
  output << "frequency_hz,output_v,normalized_db,analytical_db\n" << std::setprecision(12);
  for (int step = 0; step <= 120; ++step) {
    const double frequency = std::pow(10.0, 1.0 + step / 40.0);
    simulator.SolveAC(frequency);
    const double amplitude = std::abs(out.voltage_ac_);
    const double expected = 0.5 / std::sqrt(1.0 + std::pow(frequency / 1000.0, 10));
    if (std::abs(amplitude - expected) > 1e-7) {
      std::cerr << "Analytical validation failed at " << frequency << " Hz\n";
      return 2;
    }
    output << frequency << ',' << amplitude << ',' << 20 * std::log10(amplitude / 0.5)
           << ',' << 20 * std::log10(expected / 0.5) << '\n';
  }
}
