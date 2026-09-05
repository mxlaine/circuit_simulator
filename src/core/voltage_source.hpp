/**
 * @file voltage_source.hpp
 * @brief Defines the VoltageSource class.
 */

#ifndef VOLTAGE_SOURCE_HPP
#define VOLTAGE_SOURCE_HPP

#include <sstream>

#include "component.hpp"

/**
 * @class VoltageSource
 * @brief Represents an ideal DC voltage source.
 */
class VoltageSource : public Component {
 public:
  /**
   * @brief Constructs a voltage source.
   * @param name Component name (e.g., "V1").
   * @param value Voltage in volts (V).
   * @param posNode Pointer to the positive terminal.
   * @param negNode Pointer to the negative terminal.
   */
  VoltageSource(const std::string& name, double value, Node* posNode,
                Node* negNode)
      : Component(name, value, posNode, negNode) {}

  /**
   * @brief Returns the voltage value.
   */
  double GetValue() override { return value_; }

  /**
   * @brief Returns a descriptive string for the voltage source.
   */
  std::string info() const override {
    return "Voltage Source " + name_ + " = " + std::to_string(value_) + " V";
  }
};

#endif  // VOLTAGE_SOURCE_HPP
