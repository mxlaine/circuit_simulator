#include "netlist_parser.hpp"

#include <stdexcept>

std::unique_ptr<Circuit> NetlistParser::ParseNetlist(
    const std::string& filepath) {
  auto circuit = std::make_unique<Circuit>();

  std::ifstream file(filepath);
  if (!file.is_open()) {
    throw std::runtime_error("Unable to open netlist file: " + filepath);
  }

  std::string line;
  bool AC = false;
  bool hasCommand = false;
  while (std::getline(file, line)) {
    const auto first = line.find_first_not_of(" \t\r");
    if (first == std::string::npos || line[first] == '#') continue;
    std::stringstream iss(line);
    std::string command;
    iss >> command;

    if (command == ".DC") {
      hasCommand = true;
      AC = false;
      break;
    } else if (command == ".AC") {
      hasCommand = true;
      AC = true;
      double freq;
      if (!(iss >> freq)) {
        throw std::runtime_error("AC simulation must include frequency");
      }
      circuit->SetFrequency(freq);
      break;
    } else {
      throw std::runtime_error("Netlist must begin with simulation command");
    }
  }

  if (!hasCommand) throw std::runtime_error("Netlist must begin with simulation command");

  while (std::getline(file, line)) {
    const auto first = line.find_first_not_of(" \t\r");
    if (first == std::string::npos || line[first] == '#') continue;

    std::istringstream iss(line);
    std::string name;
    iss >> name;

    if (name.empty()) continue;

    char type = name[0];
    try {
      switch (type) {
        case 'R':
          ParseResistor(iss, name, *circuit);
          break;
        case 'C':
          ParseCapacitor(iss, name, *circuit);
          break;
        case 'L':
          ParseInductor(iss, name, *circuit, AC);
          break;
        case 'V':
          ParseVoltageSource(iss, name, *circuit);
          break;
        case 'I':
          ParseCurrentSource(iss, name, *circuit);
          break;
        case 'E':
          ParseVCVSource(iss, name, *circuit);
          break;
        case 'G':
          ParseVCCSource(iss, name, *circuit);
          break;
        case 'F':
          ParseCCCSource(iss, name, *circuit);
          break;
        case 'H':
          ParseCCVSource(iss, name, *circuit);
          break;
        case 'O':
          ParseOpAmp(iss, name, *circuit);
          break;
        default:
          throw std::runtime_error("Unsupported component: " + name);
      }
    } catch (const std::exception& e) {
      throw std::runtime_error("Error parsing line: " + line + ": " + e.what());
    }
  }
  return circuit;
}

void NetlistParser::ParseResistor(std::istringstream& iss,
                                  const std::string& name, Circuit& circuit) {
  std::string n1_name, n2_name;
  double value;
  if (!(iss >> n1_name >> n2_name >> value)) {
    throw std::runtime_error("Incorrectly formatted resistor: " + name);
  }
  Node* n1 = circuit.GetOrCreateNode(n1_name);
  Node* n2 = circuit.GetOrCreateNode(n2_name);
  circuit.AddResistor(name, value, n1, n2);
}

void NetlistParser::ParseCapacitor(std::istringstream& iss,
                                   const std::string& name, Circuit& circuit) {
  std::string n1_name, n2_name;
  double value;
  if (!(iss >> n1_name >> n2_name >> value)) {
    throw std::runtime_error("Incorrectly formatted capacitor: " + name);
  }
  Node* n1 = circuit.GetOrCreateNode(n1_name);
  Node* n2 = circuit.GetOrCreateNode(n2_name);
  circuit.AddCapacitor(name, value, n1, n2);
}

void NetlistParser::ParseInductor(std::istringstream& iss,
                                  const std::string& name, Circuit& circuit,
                                  bool AC) {
  std::string n1_name, n2_name;
  double value;
  if (!(iss >> n1_name >> n2_name >> value)) {
    throw std::runtime_error("Incorrectly formatted inductor: " + name);
  }
  Node* n1 = circuit.GetOrCreateNode(n1_name);
  Node* n2 = circuit.GetOrCreateNode(n2_name);
  if (AC == true) {
    circuit.AddInductor(name, value, n1, n2);
  } else if (AC == false) {
    circuit.AddShortCircuit(name, n1, n2);
  }
}

void NetlistParser::ParseVoltageSource(std::istringstream& iss,
                                       const std::string& name,
                                       Circuit& circuit) {
  std::string n1_name, n2_name;
  double value;
  if (!(iss >> n1_name >> n2_name >> value)) {
    throw std::runtime_error("Incorrectly formatted voltage source: " + name);
  }
  Node* n1 = circuit.GetOrCreateNode(n1_name);
  Node* n2 = circuit.GetOrCreateNode(n2_name);
  circuit.AddVoltageSource(name, value, n1, n2);
}

void NetlistParser::ParseCurrentSource(std::istringstream& iss,
                                       const std::string& name,
                                       Circuit& circuit) {
  std::string n1_name, n2_name;
  double value;
  if (!(iss >> n1_name >> n2_name >> value)) {
    throw std::runtime_error("Incorrectly formatted current source: " + name);
  }
  Node* n1 = circuit.GetOrCreateNode(n1_name);
  Node* n2 = circuit.GetOrCreateNode(n2_name);
  circuit.AddCurrentSource(name, value, n1, n2);
}

void NetlistParser::ParseVCCSource(std::istringstream& iss,
                                   const std::string& name, Circuit& circuit) {
  std::string n1_name, n2_name, ns1_name, ns2_name;
  double gm;
  if (!(iss >> n1_name >> n2_name >> ns1_name >> ns2_name >> gm)) {
    throw std::runtime_error("Incorrectly formatted VCC source: " + name);
  }
  Node* posNode = circuit.GetOrCreateNode(n1_name);
  Node* negNode = circuit.GetOrCreateNode(n2_name);
  Node* posDepNode = circuit.GetOrCreateNode(ns1_name);
  Node* negDepNode = circuit.GetOrCreateNode(ns2_name);
  circuit.AddVCCS(name, gm, posNode, negNode, posDepNode, negDepNode);
}

void NetlistParser::ParseVCVSource(std::istringstream& iss,
                                   const std::string& name, Circuit& circuit) {
  std::string n1_name, n2_name, ns1_name, ns2_name;
  double A;
  if (!(iss >> n1_name >> n2_name >> ns1_name >> ns2_name >> A)) {
    throw std::runtime_error("Incorrectly formatted VCV source: " + name);
  }
  Node* posNode = circuit.GetOrCreateNode(n1_name);
  Node* negNode = circuit.GetOrCreateNode(n2_name);
  Node* posDepNode = circuit.GetOrCreateNode(ns1_name);
  Node* negDepNode = circuit.GetOrCreateNode(ns2_name);
  circuit.AddVCVS(name, A, posNode, negNode, posDepNode, negDepNode);
}

void NetlistParser::ParseCCVSource(std::istringstream& iss,
                                   const std::string& name, Circuit& circuit) {
  std::string n1_name, n2_name, ns1_name, ns2_name;
  double Rm;
  if (!(iss >> n1_name >> n2_name >> ns1_name >> ns2_name >> Rm)) {
    throw std::runtime_error("Incorrectly formatted CCV source: " + name);
  }
  Node* posNode = circuit.GetOrCreateNode(n1_name);
  Node* negNode = circuit.GetOrCreateNode(n2_name);
  Node* posSCNode = circuit.GetOrCreateNode(ns1_name);
  Node* negSCNode = circuit.GetOrCreateNode(ns2_name);
  std::string SCname = ns1_name + ns2_name;
  ShortCircuit* sc = circuit.AddShortCircuit(SCname, posSCNode, negSCNode);
  circuit.AddCCVS(name, Rm, posNode, negNode, sc);
}

void NetlistParser::ParseCCCSource(std::istringstream& iss,
                                   const std::string& name, Circuit& circuit) {
  std::string n1_name, n2_name, ns1_name, ns2_name;
  double A;
  if (!(iss >> n1_name >> n2_name >> ns1_name >> ns2_name >> A)) {
    throw std::runtime_error("Incorrectly formatted CCC source: " + name);
  }
  Node* posNode = circuit.GetOrCreateNode(n1_name);
  Node* negNode = circuit.GetOrCreateNode(n2_name);
  Node* posSCNode = circuit.GetOrCreateNode(ns1_name);
  Node* negSCNode = circuit.GetOrCreateNode(ns2_name);
  std::string SCname = ns1_name + ns2_name;
  ShortCircuit* sc = circuit.AddShortCircuit(SCname, posSCNode, negSCNode);
  circuit.AddCCCS(name, A, posNode, negNode, sc);
}

void NetlistParser::ParseOpAmp(std::istringstream& iss, const std::string& name,
                               Circuit& circuit) {
  std::string n1_name, n2_name, n3_name;
  if (!(iss >> n1_name >> n2_name >> n3_name)) {
    throw std::runtime_error("Incorrectly formatted OpAmp: " + name);
  }
  Node* posNode = circuit.GetOrCreateNode(n1_name);
  Node* negNode = circuit.GetOrCreateNode(n2_name);
  Node* outNode = circuit.GetOrCreateNode(n3_name);
  circuit.AddOpAmp(name, posNode, negNode, outNode);
}
