#ifndef VCCS_HPP
#define VCCS_HPP

#include <sstream>

#include "component.hpp"
/**
 * @brief Voltage-Controlled Current Source: I_out = gm * V_control.
 */
class VCCS : public Component {
 public:
  /**
   * @brief Constructs VCCS.
   * @param name Identifier (e.g., "G1")
   * @param gm Transconductance in Siemens
   * @param posNode Positive output
   * @param negNode Negative output
   * @param posDepNode Positive control node
   * @param negDepNode Negative control node
   */
  VCCS(const std::string& name, double gm, Node* posNode, Node* negNode,
       Node* posDepNode, Node* negDepNode)
      : Component(name, gm, posNode, negNode),
        posDepNode_(posDepNode),
        negDepNode_(negDepNode) {}

  double GetValue() override { return value_; }

  Node* GetPosDepNode() const { return posDepNode_; }

  Node* GetNegDepNode() const { return negDepNode_; }

  std::string info() const override {
    std::ostringstream ss;
    ss << "Voltage controlled current source " << name_
       << " (Transconductance: " << value_ << " S)";
    return ss.str();
  }

 private:
  Node* posDepNode_;
  Node* negDepNode_;
};

#endif  // VCCS_HPP