#ifndef VCVS_HPP
#define VCVS_HPP

#include <sstream>

#include "component.hpp"
/**
 * @brief Voltage-Controlled Voltage Source: V_out = A * V_control.
 */
class VCVS : public Component {
 public:
  /**
   * @brief Constructs VCVS.
   * @param name Identifier (e.g., "E1")
   * @param A Voltage gain
   * @param posNode Positive output
   * @param negNode Negative output
   * @param posDepNode Positive control node
   * @param negDepNode Negative control node
   */
  VCVS(const std::string& name, double A, Node* posNode, Node* negNode,
       Node* posDepNode, Node* negDepNode)
      : Component(name, A, posNode, negNode),
        posDepNode_(posDepNode),
        negDepNode_(negDepNode) {}

  double GetValue() override { return value_; }

  Node* GetPosDepNode() const { return posDepNode_; }

  Node* GetNegDepNode() const { return negDepNode_; }

  std::string info() const override {
    std::ostringstream ss;
    ss << "Voltage controlled voltage source " << name_ << " (Gain: " << value_
       << ")";
    return ss.str();
  }

 private:
  Node* posDepNode_;
  Node* negDepNode_;
};

#endif  // VCVS_HPP