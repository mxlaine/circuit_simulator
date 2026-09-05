#ifndef CCVS_HPP
#define CCVS_HPP

#include <sstream>

#include "component.hpp"
#include "short_circuit.hpp"
/**
 * @brief Current-Controlled Voltage Source (CCVS)
 * V_out = Rm * I_control, where Rm is the transresistance.
 */
class CCVS : public Component {
 public:
  /**
   * @brief Constructs a current-controlled voltage source.
   *
   * @param name Unique identifier for the CCVS (e.g., "H1")
   * @param Rm Transresistance value in Ohms
   * @param posNode Pointer to the positive output terminal
   * @param negNode Pointer to the negative output terminal
   * @param sc Pointer to the short circuit that measures the controlling
   * current
   */
  CCVS(const std::string& name, double Rm, Node* posNode, Node* negNode,
       ShortCircuit* sc)
      : Component(name, Rm, posNode, negNode), short_circuit_(sc) {}

  double GetValue() override { return value_; }

  ShortCircuit* GetShortCircuit() const { return short_circuit_; }

  std::string info() const override {
    std::ostringstream ss;
    ss << "Current controlled voltage source " << name_
       << " (Transresistance: " << value_ << " Ω)";
    return ss.str();
  }

 private:
  ShortCircuit* short_circuit_;
};

#endif  // CCCS_HPP