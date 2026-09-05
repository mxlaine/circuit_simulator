/**
 * @file resistor.hpp
 * @brief Defines the Resistor class.
 */

#ifndef RESISTOR_HPP
#define RESISTOR_HPP

#include "component.hpp"

/**
 * @class Resistor
 * @brief Represents an ideal resistor component.
 */
class Resistor : public Component {
 public:
  /**
   * @brief Constructs a resistor.
   * @param name Component name (e.g., "R1").
   * @param value Resistance in ohms (Ω).
   * @param posNode Pointer to the positive terminal.
   * @param negNode Pointer to the negative terminal.
   */
  Resistor(const std::string& name, double value, Node* posNode, Node* negNode)
      : Component(name, value, posNode, negNode) {}

  /**
   * @brief Returns the conductance (1/R).
   * @return Conductance in siemens (S).
   */
  double GetValue() override { return (value_ != 0.0) ? 1.0 / value_ : 0.0; }

  /**
   * @brief Returns a descriptive string for the resistor.
   */
  std::string info() const override {
    std::ostringstream ss;
    ss << "Resistor " << name_ << " (" << value_ << " ohm)";
    return ss.str();
  }
};

#endif  // RESISTOR_HPP
