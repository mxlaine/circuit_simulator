#ifndef SHORT_CIRCUIT_HPP
#define SHORT_CIRCUIT_HPP

#include "voltage_source.hpp"
/**
 * @brief Zero-voltage source for current measurement.
 *
 * Used with current-controlled sources (CCCS, CCVS).
 */
class ShortCircuit : public VoltageSource {
 public:
  /**
   * @brief Constructs short circuit (0V source).
   */
  ShortCircuit(const std::string& name, Node* posNode, Node* negNode)
      : VoltageSource(name, 0, posNode, negNode) {}

  std::string info() const override {
    std::ostringstream ss;
    ss << "Short circuit " << name_;
    return ss.str();
  }
};

#endif  // SHORT_CIRCUIT_HPP