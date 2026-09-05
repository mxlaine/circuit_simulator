#include <cmath>
#include <complex>
#include <iostream>
#include <vector>

#include "core/capacitor.hpp"
#include "core/circuit.hpp"
#include "core/component.hpp"
#include "core/current_source.hpp"
#include "core/inductor.hpp"
#include "core/node.hpp"
#include "core/resistor.hpp"
#include "core/voltage_source.hpp"
#include "simulator.hpp"

// --- Utility printing functions ---

void printCurrentsDC(const std::vector<Component*>& components) {
  for (auto* comp : components)
    std::cout << comp->info() << " current: " << comp->current_ << " A\n";
}

void printCurrentsAC(const std::vector<Component*>& components) {
  for (auto* comp : components) {
    std::complex<double> i = comp->complex_current_;
    double mag = std::abs(i);
    double phase = std::arg(i) * 180.0 / M_PI;
    std::cout << comp->info() << " current: |I| = " << mag
              << " A, ∠ = " << phase << "°\n";
  }
}

void printNodeVoltagesDC(const std::vector<Node*>& nodes) {
  for (auto node : nodes)
    std::cout << "Node " << node->GetName() << ": " << node->voltage_ << " V\n";
}

void printNodeVoltagesAC(const std::vector<Node*>& nodes) {
  for (auto node : nodes) {
    std::complex<double> v = node->voltage_ac_;
    double mag = std::abs(v);
    double phase = std::arg(v) * 180.0 / M_PI;
    std::cout << "Node " << node->GetName() << ": |V| = " << mag
              << " V, ∠ = " << phase << "°\n";
  }
}

void printDCVoltageDrops(const std::vector<Component*>& components) {
  std::cout << "\nDC Voltage Drops\n";
  for (auto* comp : components) {
    double vp = comp->GetPosNode()->voltage_;
    double vn = comp->GetNegNode()->voltage_;
    double drop = vp - vn;
    std::cout << comp->info() << " Vdrop = " << drop << " V\n";
  }
}

void printACVoltageDrops(const std::vector<Component*>& components) {
  std::cout << "\nAC Voltage Drops\n";
  for (auto* comp : components) {
    auto v = comp->GetVoltageDropAC();
    double mag = std::abs(v);
    double phase = std::arg(v) * 180.0 / M_PI;
    std::cout << comp->info() << " |Vdrop| = " << mag << " V ∠ " << phase
              << "°\n";
  }
}

void printSineWave(Node* node, double frequency, double sample_rate,
                   double duration) {
  std::vector<double> samples;
  double omega = 2.0 * M_PI * frequency;
  double phi = std::arg(node->voltage_ac_);
  double V = std::abs(node->voltage_ac_);
  int N = static_cast<int>(duration * sample_rate);

  samples.reserve(N);
  for (int n = 0; n < N; ++n) {
    double t = n / sample_rate;
    samples.push_back(V * std::sin(omega * t + phi));
  }

  std::cout << "Sine wave for Node " << node->GetName() << ":\n";
  for (int n = 0; n < std::min(10, N); ++n)
    std::cout << "t=" << n / sample_rate << " s: " << samples[n] << " V\n";
}

void runVCVSTestWithSource() {
  std::cout << "\n--- VCVS Test (DC + AC) ---\n";

  Node gnd(0, "GND");
  Node n1(1), n2(2), n3(3);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();

  // Resistors
  circuit->AddResistor("R1", 1000.0, &gnd, &n1);
  circuit->AddResistor("R2", 2000.0, &n1, &n2);
  circuit->AddResistor("R3", 3000.0, &n2, &n3);

  // DC & AC voltage source
  circuit->AddVoltageSource("V1", 10.0, &n3, &gnd);

  // VCVS: output n2-n1, control n2-n3, gain=91
  circuit->AddVCVS("A1", 91.0, &n2, &n1, &n2, &n3);

  circuit->extractNodes();
  Simulator sim(circuit.get());
  auto nodes = circuit->GetNodes();
  auto components = circuit->GetComponents();

  // --- DC ---
  std::cout << "\n--- DC Operating Point ---\n";
  sim.SolveDC();
  printNodeVoltagesDC(nodes);
  printCurrentsDC(components);
  printDCVoltageDrops(components);

  // --- AC ---
  std::cout << "\n--- AC Analysis (1 kHz) ---\n";
  sim.SolveAC(1000.0);
  printNodeVoltagesAC(nodes);
  printCurrentsAC(components);
  printACVoltageDrops(components);
}

void runVCCSTestWithSource() {
  std::cout << "\n--- VCCS Test (DC + AC) ---\n";

  Node gnd(0, "GND");
  Node n1(1), n2(2), n3(3);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();

  // Resistors
  circuit->AddResistor("R1", 1000.0, &gnd, &n1);
  circuit->AddResistor("R2", 2000.0, &n1, &n2);
  circuit->AddResistor("R3", 3000.0, &n2, &n3);

  // Voltage source
  circuit->AddVoltageSource("V1", 10.0, &n3, &gnd);

  // VCCS: output current n2->n1, control voltage n2-n3, G=0.01 S
  circuit->AddVCCS("G1", 0.01, &n2, &n1, &n2, &n3);

  circuit->extractNodes();
  Simulator sim(circuit.get());
  auto nodes = circuit->GetNodes();
  auto components = circuit->GetComponents();

  // --- DC ---
  std::cout << "\n--- DC Operating Point ---\n";
  sim.SolveDC();
  printNodeVoltagesDC(nodes);
  printCurrentsDC(components);
  printDCVoltageDrops(components);

  // --- AC ---
  std::cout << "\n--- AC Analysis (1 kHz) ---\n";
  sim.SolveAC(1000.0);
  printNodeVoltagesAC(nodes);
  printCurrentsAC(components);
  printACVoltageDrops(components);
}

void runCCCSTestWithSource() {
  std::cout << "\n--- CCCS Test (DC + AC) ---\n";

  Node gnd(0, "GND");
  Node n1(1), n2(2), n3(3), x(4);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();

  // Resistors
  circuit->AddResistor("R1", 1000.0, &gnd, &n1);
  circuit->AddResistor("R2", 2000.0, &n1, &n2);
  circuit->AddResistor("R3", 8000.0, &n2, &n3);

  // Voltage source
  circuit->AddVoltageSource("V1", 10.0, &n3, &x);
  ShortCircuit* sc = circuit->AddShortCircuit("SC1", &x, &gnd);

  // CCCS: output current n2->n1, controlled by current through SC1, gain=2
  circuit->AddCCCS("F1", 2.0, &n2, &n1, sc);

  circuit->extractNodes();
  Simulator sim(circuit.get());
  auto nodes = circuit->GetNodes();
  auto components = circuit->GetComponents();

  // --- DC ---
  std::cout << "\n--- DC Operating Point ---\n";
  sim.SolveDC();
  printNodeVoltagesDC(nodes);
  printCurrentsDC(components);
  printDCVoltageDrops(components);

  // --- AC ---
  std::cout << "\n--- AC Analysis (1 kHz) ---\n";
  sim.SolveAC(1000.0);
  printNodeVoltagesAC(nodes);
  printCurrentsAC(components);
  printACVoltageDrops(components);
}

void runCCVSTestWithSource() {
  std::cout << "\n--- CCVS Test (DC + AC) ---\n";

  Node gnd(0, "GND");
  Node n1(1), n2(2), n3(3), x(4);

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();

  // Resistors
  circuit->AddResistor("R1", 1000.0, &gnd, &n1);
  circuit->AddResistor("R2", 2000.0, &n1, &n2);
  circuit->AddResistor("R3", 8000.0, &n2, &n3);

  // Voltage source
  circuit->AddVoltageSource("V1", 10.0, &x, &gnd);
  ShortCircuit* sc = circuit->AddShortCircuit("SC2", &n3, &x);

  // CCVS: output voltage n2-n1, controlled by current through SC2, gain=50Ω
  circuit->AddCCVS("H1", 50.0, &n2, &n1, sc);

  circuit->extractNodes();
  Simulator sim(circuit.get());
  auto nodes = circuit->GetNodes();
  auto components = circuit->GetComponents();

  // --- DC ---
  std::cout << "\n--- DC Operating Point ---\n";
  sim.SolveDC();
  printNodeVoltagesDC(nodes);
  printCurrentsDC(components);
  printDCVoltageDrops(components);

  // --- AC ---
  std::cout << "\n--- AC Analysis (1 kHz) ---\n";
  sim.SolveAC(1000.0);
  printNodeVoltagesAC(nodes);
  printCurrentsAC(components);
  printACVoltageDrops(components);
}

void runMultiBranchCircuit() {
  Node gnd(0, "GND");
  Node n1(1), n2(2), n3(3);

  // --- Components ---
  // 5 V DC source between node 1 and ground
  // RLC series between node 1 → node 2 → node 3 → ground
  Resistor* r1 = new Resistor("R1", 1000.0, &n1, &n2);   // 1 kΩ
  Inductor* l1 = new Inductor("L1", 1e-3, &n2, &n3);     // 1 mH
  Capacitor* c1 = new Capacitor("C1", 1e-6, &n2, &gnd);  // 1 µF
  Resistor* r2 = new Resistor("R2", 1000, &n3, &gnd);

  std::vector<Component*> components = {r1, l1, c1, r2};

  // --- Build circuit ---
  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>(components);
  circuit->extractNodes();

  // --- Add DC voltage source ---
  circuit->AddVoltageSource("V1", 5.0, &n1, &gnd);

  // --- Prepare simulator ---
  Simulator sim(circuit.get());

  // --- Build matrices ---
  std::vector<Node*> nodes = circuit->GetNodes();
  Eigen::MatrixXd G(nodes.size(), nodes.size());
  sim.FillGCMatrix(G, components);
  std::cout << "G matrix (conductances):\n" << G << "\n\n";

  // --- DC solve ---
  std::cout << "Running DC operating point simulation...\n";
  sim.SolveDC();
  std::cout << "\nDC Operating Point\n";
  printNodeVoltagesDC(nodes);
  printCurrentsDC(components);
  printDCVoltageDrops(components);

  std::cout << "\nNode voltages (DC):\n";
  for (auto node : nodes) {
    std::cout << "  Node " << node->GetName() << ": " << node->voltage_
              << " V\n";
  }

  std::cout << "\nResistor currents:\n";
  for (auto r : circuit->GetResistors()) {
    std::cout << "  " << r->info() << " -> I = " << r->current_ << " A\n";
  }

  std::cout << "\nInductors:\n";
  for (auto l : circuit->GetInductors()) {
    std::cout << "  " << l->info() << "\n";
  }

  std::cout << "\nCapacitors:\n";
  for (auto c : circuit->GetCapacitors()) {
    std::cout << "  " << c->info() << "\n";
  }
  // --- AC analysis ---
  std::cout << "\nRunning AC analysis at 1 kHz...\n";
  sim.SolveAC(1000.0);
  std::cout << "\nAC Analysis at 1 kHz\n";
  printNodeVoltagesAC(nodes);
  printCurrentsAC(components);
  printACVoltageDrops(components);
}

void runOpAmpTest() {
  std::cout << "\n--- OpAmp Non-Inverting Amplifier Test ---\n";

  // Create nodes
  Node gnd(0, "GND");
  Node n1(1);  // Input signal node
  Node n2(2);  // OpAmp positive input (connected to input)
  Node n3(3);  // OpAmp negative input (feedback node)
  Node n4(4);  // OpAmp output

  std::unique_ptr<Circuit> circuit = std::make_unique<Circuit>();

  // --- Input voltage source ---
  circuit->AddVoltageSource("Vin", 1.0, &n1, &gnd);  // 1 V input

  // --- Input resistor ---
  circuit->AddResistor("Rin", 10000.0, &n1, &n2);  // 10kΩ

  // --- Feedback network ---
  circuit->AddResistor("R1", 1000.0, &n3, &gnd);  // 1kΩ to ground
  circuit->AddResistor("R2", 9000.0, &n4, &n3);   // 9kΩ feedback

  // --- OpAmp ---
  // Positive input at n2, negative input at n3, output at n4
  circuit->AddOpAmp("OA1", &n2, &n3, &n4);

  // --- Load resistor ---
  circuit->AddResistor("RL", 10000.0, &n4, &gnd);  // 10kΩ load

  // Extract nodes and prepare simulator
  circuit->extractNodes();
  Simulator sim(circuit.get());
  auto nodes = circuit->GetNodes();
  auto components = circuit->GetComponents();

  // --- Fill matrix ---
  Eigen::MatrixXd A;
  sim.FillAMatrix(A);
  std::cout << "\nCircuit matrix A (" << A.rows() << "x" << A.cols() << "):\n"
            << A << "\n";

  // --- DC Solve ---
  sim.SolveDC();
  std::cout << "\n--- DC Operating Point ---\n";
  printNodeVoltagesDC(nodes);
  printCurrentsDC(components);
  printDCVoltageDrops(components);

  // Calculate theoretical gain
  double R1 = 1000.0;
  double R2 = 9000.0;
  double theoretical_gain = 1.0 + (R2 / R1);
  double measured_gain = n4.voltage_ / 1.0;  // Output / Input

  std::cout << "\n--- Amplifier Analysis ---\n";
  std::cout << "Input voltage: 1.0 V\n";
  std::cout << "Output voltage: " << n4.voltage_ << " V\n";
  std::cout << "Theoretical gain: " << theoretical_gain << "\n";
  std::cout << "Measured gain: " << measured_gain << "\n";

  // --- AC Solve ---
  std::cout << "\n--- AC Analysis (1 kHz) ---\n";
  sim.SolveAC(1000.0);
  printNodeVoltagesAC(nodes);
  printCurrentsAC(components);
  printACVoltageDrops(components);
}

int main() {
  runVCCSTestWithSource();
  runVCVSTestWithSource();
  runCCCSTestWithSource();
  runCCVSTestWithSource();
  runMultiBranchCircuit();
  runOpAmpTest();

  return 0;
}