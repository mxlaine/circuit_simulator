#ifndef CCCS_HPP
#define CCCS_HPP

#include <sstream>

#include "component.hpp"
#include "short_circuit.hpp"
/**
 * @brief Current-Controlled Current Source (CCCS)
 * I_out = A * I_control, where A is the current gain.
 */
class CCCS : public Component {
 public:
  /**
   * @brief Constructs a current-controlled current source.
   *
   * @param name Unique identifier for the CCCS (e.g., "F1")
   * @param A Current gain (dimensionless ratio)
   * @param posNode Pointer to the positive output terminal
   * @param negNode Pointer to the negative output terminal
   * @param sc Pointer to the short circuit that measures the controlling
   * current
   */
  CCCS(const std::string& name, double A, Node* posNode, Node* negNode,
       ShortCircuit* sc)
      : Component(name, A, posNode, negNode), short_circuit_(sc) {}

  double GetValue() override { return value_; }

  ShortCircuit* GetShortCircuit() const { return short_circuit_; }

  std::string info() const override {
    std::ostringstream ss;
    ss << "Current controlled current source " << name_ << " (Gain: " << value_
       << ")";
    return ss.str();
  }

 private:
  ShortCircuit* short_circuit_;
};

#endif  // CCCS_HPP