/**
 * @file current_source.hpp
 * @brief Defines the CurrentSource class.
 */

#ifndef CURRENT_SOURCE_HPP
#define CURRENT_SOURCE_HPP

#include <sstream>

#include "component.hpp"

/**
 * @class CurrentSource
 * @brief Represents an ideal DC current source.
 */
class CurrentSource : public Component {
 public:
  /**
   * @brief Constructs a current source.
   * @param name Component name (e.g., "I1").
   * @param value Current in amperes (A).
   * @param posNode Pointer to the positive terminal.
   * @param negNode Pointer to the negative terminal.
   */
  CurrentSource(const std::string& name, double value, Node* posNode,
                Node* negNode)
      : Component(name, value, posNode, negNode) {}

  /**
   * @brief Returns the current value.
   */
  double GetValue() override { return value_; }

  /**
   * @brief Returns a descriptive string for the current source.
   */
  std::string info() const override {
    return "Current Source " + name_ + " = " + std::to_string(value_) + " A";
  }
};

#endif  // CURRENT_SOURCE_HPP
