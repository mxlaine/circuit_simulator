#ifndef OPAMP_HPP
#define OPAMP_HPP

#include <sstream>

#include "component.hpp"
#include "node.hpp"
/**
  @brief Ideal operational amplifier model.
*/
class OpAmp : public Component {
 public:
  /**
    @brief Constructs an ideal operational amplifier.
    @param name Unique identifier for the op-amp (e.g., "OA1")
    @param posNode Pointer to the non-inverting input node (V+)
    @param negNode Pointer to the inverting input node (V-)
    @param outNode Pointer to the output node
  */
  OpAmp(const std::string& name, Node* posNode, Node* negNode, Node* outNode)
      : Component(name, 0, posNode, negNode), outNode_(outNode) {}

  double GetValue() override { return value_; }

  Node* GetOutNode() { return outNode_; }

  virtual std::string info() const override {
    std::ostringstream ss;
    ss << "Operational amplifier " << name_;
    return ss.str();
  }

 private:
  Node* outNode_;
};

#endif
