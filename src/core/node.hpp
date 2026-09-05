/**
 * @file node.hpp
 * @brief Defines the Node class used to represent circuit connection points.
 */

#ifndef NODE_HPP
#define NODE_HPP

#include <complex>
#include <string>

/**
 * @class Node
 * @brief Represents an electrical node (connection point) in a circuit.
 *
 * Each node serves as a connection between multiple components in a circuit.
 * Nodes are uniquely identified by an index and may optionally have a name.
 * The simulator assigns voltages to nodes during analysis.
 */
/**
 * @brief Represents an electrical node in the circuit.
 */
class Node {
 public:
  /**
   * @brief Constructs a Node with an optional name.
   * @param index Unique index identifying the node.
   * @param name Optional node name (e.g., "1", "GND").
   */
  Node(int index, const std::string& name) : index_(index), name_(name) {}
  Node(int index) : index_(index), name_(std::to_string(index)) {}
  Node(const std::string& name) : index_(std::stoi(name)), name_(name) {}
  /**
   * @brief Returns the node's index.
   * @return Integer node index.
   */
  int GetIndex() const { return index_; }
  /**
   * @brief Returns the node's name.
   * @return Name of the node as a string.
   */
  std::string GetName() const { return name_; }

  int index_;
  std::string name_;
  double voltage_ = 0.0;                   ///< DC node voltage
  std::complex<double> voltage_ac_ = 0.0;  ///< AC node voltage (phasor)
};

#endif  // NODE_HPP
