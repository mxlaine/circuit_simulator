#ifndef CIRCUIT_HPP
#define CIRCUIT_HPP

#include <set>
#include <string>
#include <vector>

#include "capacitor.hpp"
#include "cccs.hpp"
#include "ccvs.hpp"
#include "component.hpp"
#include "inductor.hpp"
#include "node.hpp"
#include "opamp.hpp"
#include "resistor.hpp"
#include "short_circuit.hpp"
#include "vccs.hpp"
#include "vcvs.hpp"

class Circuit {
 public:
  Circuit() = default;

  Circuit(std::vector<Component*>& components);

  ~Circuit();

  /**
   * @brief Get a vector of unique nodes from the
   * components in the circuit
   *
   */
  void extractNodes();
  std::vector<Component*> GetComponents() const;

  /**
   * @brief Adds resistor to the circuit
   *
   * @param n name of resistor
   * @param n1 node 1
   * @param n2 node 2
   * @param r resistance
   */
  void AddResistor(const std::string& n, double r, Node* n1, Node* n2);

  /**
   * @brief Adds capacitor to the circuit
   *
   * @param n name of capacitor
   * @param c capacitance
   * @param n1 node 1
   * @param n2 node 2
   */
  void AddCapacitor(const std::string& n, double c, Node* n1, Node* n2);

  /**
   * @brief Adds inductor to the circuit
   *
   * @param n name of inductor
   * @param L inductance
   * @param n1 node 1
   * @param n2 node 2
   */
  void AddInductor(const std::string& n, double L, Node* n1, Node* n2);

  /**
   * @brief Adds voltage source to the circuit
   *
   * @param n name of source
   * @param v voltage
   * @param n1 node 1
   * @param n2 node 2
   */
  void AddVoltageSource(const std::string& n, double v, Node* n1, Node* n2);

  /**
   * @brief Adds current source to the circuit
   *
   * @param n name of source
   * @param i current
   * @param n1 node 1
   * @param n2 node 2
   */
  void AddCurrentSource(const std::string& n, double i, Node* n1, Node* n2);

  /**
   * @brief Adds voltage controlled voltage source to the circuit
   *
   * @param n name of source
   * @param A Gain
   * @param n1 node 1
   * @param n2 node 2
   * @param depn1 dependent node 1
   * @param depn2 dependent node 2
   */
  void AddVCVS(const std::string& n, double A, Node* n1, Node* n2, Node* depn1,
               Node* depn2);

  /**
   * @brief Adds voltage controlled current source to the circuit
   *
   * @param n name of source
   * @param gm Transconductance
   * @param n1 node 1
   * @param n2 node 2
   * @param depn1 dependent node 1
   * @param depn2 dependent node 2
   */
  void AddVCCS(const std::string& n, double gm, Node* n1, Node* n2, Node* depn1,
               Node* depn2);

  /**
   * @brief Adds current controlled current source to the circuit
   *
   * @param n name of source
   * @param A Gain
   * @param n1 node 1
   * @param n2 node 2
   * @param sc dependent short circuit
   */
  void AddCCCS(const std::string& n, double A, Node* n1, Node* n2,
               ShortCircuit* sc);

  /**
   * @brief Adds current controlled voltage source to the circuit
   *
   * @param n name of source
   * @param Rm Transresistance
   * @param n1 node 1
   * @param n2 node 2
   * @param sc dependent short circuit
   */
  void AddCCVS(const std::string& n, double Rm, Node* n1, Node* n2,
               ShortCircuit* sc);
  /**
   * @brief Adds short circuit to the circuit
   *
   * @param n name
   * @param n1 node 1
   * @param n2 node 2
   */
  ShortCircuit* AddShortCircuit(const std::string& n, Node* n1, Node* n2);

  /**
   * @brief Adds Op Amp to the circuit
   *
   * @param n name
   * @param n1 node 1
   * @param n2 node 2
   * @param outnode output node
   */
  void AddOpAmp(const std::string& n, Node* n1, Node* n2, Node* outnode);

  /**
   * @brief Get the resistors in the circuit
   *
   * @return std::vector<Component*> vector of resistors
   */
  std::vector<Component*>& GetResistors() { return resistors_; }

  /**
   * @brief Get the capacitors in the circuit
   *
   * @return std::vector<Component*> vector of capacitors
   */
  std::vector<Component*>& GetCapacitors() { return capacitors_; }

  /**
   * @brief Get the inductors in the circuit
   *
   * @return std::vector<Component*> vector of inductors
   */
  std::vector<Component*>& GetInductors() { return inductors_; }

  /**
   * @brief Get the voltage sources in the circuit
   *
   * @return std::vector<Component*>& vector of voltage sources
   */
  std::vector<Component*>& GetVoltageSources() { return voltage_sources_; }

  /**
   * @brief Get the current sources in the circuit
   *
   * @return std::vector<Component*>& vector of current sources
   */
  std::vector<Component*>& GetCurrentSources() { return current_sources_; }

  /**
   * @brief Get the voltage controlled voltage sources in the circuit
   *
   * @return std::vector<Component*>& vector of VCVSs
   */
  std::vector<VCVS*>& GetVCVSs() { return VCVSs_; }

  /**
   * @brief Get the voltage controlled current sources in the circuit
   *
   * @return std::vector<Component*>& vector of VCCSs
   */
  std::vector<VCCS*>& GetVCCSs() { return VCCSs_; }

  /**
   * @brief Get the current controlled current sources in the circuit
   *
   * @return std::vector<Component*>& vector of CCCSs
   */
  std::vector<CCCS*>& GetCCCSs() { return CCCSs_; }

  /**
   * @brief Get the current controlled voltage sources in the circuit
   *
   * @return std::vector<Component*>& vector of CCVSs
   */
  std::vector<CCVS*>& GetCCVSs() { return CCVSs_; }

  /**
   * @brief Get the Op Amps in the circuit
   *
   * @return std::vector<OpAmp*> vector of Op Amps
   */
  std::vector<OpAmp*>& GetOpAmps() { return opamps_; }

  /**
   * @brief Get the nodes in the circuit
   *
   * @return std::vector<Node*>& vector of nodes
   */
  std::vector<Node*> GetNodes() {
    return std::vector<Node*>(nodes_.begin(), nodes_.end());
  }

  /**
   * @brief Compares Nodes in set by name, rather than pointer
   *
   * @brief Compares Nodes in set by name, rather than pointer
   *
   */
  struct NodeComparator {
    bool operator()(const Node* a, const Node* b) const {
      return a->GetName() < b->GetName();
    }
  };

  /**
   * @brief Creates new or returns already existing Node
   *
   * @param name Name of node
   * @brief Creates new or returns already existing Node
   *
   * @param name Name of node
   * @return Node* pointer to Node object
   */
  Node* GetOrCreateNode(const std::string& name);

  /**
   * @brief Set the Frequency for AC simulation
   *
   * @param freq frequency
   */
  void SetFrequency(double freq) { frequency_ = freq; }

  /**
   * @brief Get the Frequency value
   *
   * @return double
   */
  double GetFrequency() const { return frequency_; }

  bool HasGND() const;

  Node* GetGND() const { return GND_; }

 private:
  std::vector<Component*> resistors_;
  std::vector<Component*> capacitors_;
  std::vector<Component*> inductors_;
  std::vector<Component*> voltage_sources_;
  std::vector<Component*> current_sources_;
  std::vector<VCVS*> VCVSs_;
  std::vector<VCCS*> VCCSs_;
  std::vector<CCCS*> CCCSs_;
  std::vector<CCVS*> CCVSs_;
  std::vector<OpAmp*> opamps_;
  std::set<Node*, NodeComparator> nodes_;
  double frequency_ = 0.0;
  bool hasGND_ = false;
  Node* GND_;
};

#endif  // CIRCUIT_HPP
