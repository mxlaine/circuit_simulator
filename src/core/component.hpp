#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include <complex>
#include <string>

#include "node.hpp"

class Node;

/**
 * @brief Abstract base class for all electrical components.
 */
class Component {
 public:
  /**
   * @brief Default constructor.
   */
  Component() = default;

  /**
   * @brief Constructs a component with the given parameters.
   * @param name Name of the component.
   * @param value Numeric value (resistance, capacitance, etc.).
   * @param posNode Pointer to the positive terminal node.
   * @param negNode Pointer to the negative terminal node.
   */
  Component(const std::string& name, double value = 0.0,
            Node* posNode = nullptr, Node* negNode = nullptr)
      : value_(value), name_(name), posNode_(posNode), negNode_(negNode) {}

  /**
   * @brief Virtual destructor.
   */
  virtual ~Component() = default;

  /**
   * @brief Returns a pointer to the positive terminal node.
   * @return Pointer to the positive node.
   */
  Node* GetPosNode() { return posNode_; };

  /**
   * @brief Returns a pointer to the negative terminal node.
   * @return Pointer to the negative node.
   */
  Node* GetNegNode() { return negNode_; };

  /**
   * @brief Returns the effective value of the component.
   *
   * For example, resistors return conductance (1/R),
   * while other components return their raw stored value.
   *
   * @return Component’s value or equivalent quantity.
   */
  virtual double GetValue() = 0;

  /**
   * @brief Returns a string with human-readable information about the
   * component.
   * @return Descriptive string including name and value.
   */
  virtual std::string info() const = 0;

  /** @brief DC or transient scalar current value. */
  double current_ = 0.0;

  /**
   * @brief AC steady-state complex current (phasor).
   *
   * Contains both magnitude and phase of the current for frequency-domain
   * analysis.
   */
  std::complex<double> complex_current_ = {0.0, 0.0};

  double GetVoltageDropDC() const {
    if (!posNode_ || !negNode_) return 0.0;
    return posNode_->voltage_ - negNode_->voltage_;
  }

  std::complex<double> GetVoltageDropAC() const {
    if (!posNode_ || !negNode_) return {0.0, 0.0};
    return posNode_->voltage_ac_ - negNode_->voltage_ac_;
  }

 protected:
  double value_;             ///< Numeric value of the component (R, L, C, etc.)
  std::string name_;         ///< Component name (e.g., "R1", "C2").
  Node* posNode_ = nullptr;  ///< Pointer to positive terminal node.
  Node* negNode_ = nullptr;  ///< Pointer to negative terminal node.
};

#endif  // COMPONENT_HPP
