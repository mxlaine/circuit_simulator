/**
 * @file capacitor.hpp
 * @brief Defines the Capacitor class.
 */

#ifndef CAPACITOR_HPP
#define CAPACITOR_HPP

#include <sstream>

#include "component.hpp"

/**
 * @class Capacitor
 * @brief Represents an ideal capacitor component.
 */
class Capacitor : public Component {
 public:
  /**
   * @brief Constructs a capacitor.
   * @param name Component name (e.g., "C1").
   * @param value Capacitance in farads (F).
   * @param posNode Pointer to the positive terminal.
   * @param negNode Pointer to the negative terminal.
   */
  Capacitor(const std::string& name, double value, Node* posNode, Node* negNode)
      : Component(name, value, posNode, negNode) {}

  /**
   * @brief Returns the capacitance value.
   */
  double GetValue() override { return value_; }

  /**
   * @brief Returns a descriptive string for the capacitor.
   */
  std::string info() const override {
    return "Capacitor " + name_ + " = " + std::to_string(value_) + " F";
  }
};

#endif  // CAPACITOR_HPP
