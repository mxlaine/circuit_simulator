/**
 * @file inductor.hpp
 * @brief Defines the Inductor class.
 */

#ifndef INDUCTOR_HPP
#define INDUCTOR_HPP

#include <sstream>

#include "component.hpp"

/**
 * @class Inductor
 * @brief Represents an ideal inductor component.
 */
class Inductor : public Component {
 public:
  /**
   * @brief Constructs an inductor.
   * @param name Component name (e.g., "L1").
   * @param value Inductance in henrys (H).
   * @param posNode Pointer to the positive terminal.
   * @param negNode Pointer to the negative terminal.
   */
  Inductor(const std::string& name, double value, Node* posNode, Node* negNode)
      : Component(name, value, posNode, negNode) {}

  /**
   * @brief Returns the inductance value.
   */
  double GetValue() override { return value_; }

  /**
   * @brief Returns a descriptive string for the inductor.
   */
  std::string info() const override {
    return "Inductor " + name_ + " = " + std::to_string(value_) + " H";
  }
};

#endif  // INDUCTOR_HPP
