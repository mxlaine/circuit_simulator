#ifndef NETLIST_PARSER_HPP
#define NETLIST_PARSER_HPP

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

#include "circuit.hpp"
#include "component.hpp"

class NetlistParser {
 public:
  /**
   * @brief Construct a new Netlist Parser object
   *
   */
  NetlistParser() = default;

  /**
   * @brief Creates a Circuit object from a netlist
   *
   * @param filepath path to the netlist file
   */
  std::unique_ptr<Circuit> ParseNetlist(const std::string& filepath);

 private:
  /**
   * @brief Creates a Resistor object from the given line
   *
   * @param iss input stream including the line
   * @param name name of Resistor
   * @param circuit reference to Circuit being created
   */
  void ParseResistor(std::istringstream& iss, const std::string& name,
                     Circuit& circuit);

  /**
   * @brief Creates a Capacitor object from the given line
   *
   * @param iss input stream including the line
   * @param name name of Capacitor
   * @param circuit reference to Circuit being created
   */
  void ParseCapacitor(std::istringstream& iss, const std::string& name,
                      Circuit& circuit);

  /**
   * @brief Crates a Inductor object from the given line
   *
   * @param iss input stream including the line
   * @param name name of Inductor
   * @param circuit reference to Circuit being created
   */
  void ParseInductor(std::istringstream& iss, const std::string& name,
                     Circuit& circuit, bool AC);

  /**
   * @brief Creates a VoltageSource object from the given line
   *
   * @param iss input stream including the line
   * @param name name of VoltageSource
   * @param circuit reference to Circuit being created
   */
  void ParseVoltageSource(std::istringstream& iss, const std::string& name,
                          Circuit& circuit);

  /**
   * @brief Creates a CurrentSource object from the given line
   *
   * @param iss input stream including the line
   * @param name name of CurrentSource
   * @param circuit reference to Circuit being created
   */
  void ParseCurrentSource(std::istringstream& iss, const std::string& name,
                          Circuit& circuit);

  /**
   * @brief Creates a VCCS object from the given line
   *
   * @param iss input stream including the line
   * @param name name of VCCS
   * @param circuit reference to Circuit being created
   */
  void ParseVCCSource(std::istringstream& iss, const std::string& name,
                      Circuit& circuit);

  /**
   * @brief Creates a VCVS object from the given line
   *
   * @param iss input stream including the line
   * @param name name of VCVS
   * @param circuit reference to Circuit being created
   */
  void ParseVCVSource(std::istringstream& iss, const std::string& name,
                      Circuit& circuit);

  /**
   * @brief Creates a CCVS object from the given line
   *
   * @param iss input stream including the line
   * @param name name of CCVS
   * @param circuit reference to Circuit being created
   */
  void ParseCCVSource(std::istringstream& iss, const std::string& name,
                      Circuit& circuit);

  /**
   * @brief Creates a CCCS object from the given line
   *
   * @param iss input stream including the line
   * @param name name of CCCS
   * @param circuit reference to Circuit being created
   */
  void ParseCCCSource(std::istringstream& iss, const std::string& name,
                      Circuit& circuit);

  /**
   * @brief Creates a OpAmp opject from the given line
   *
   * @param iss input stream including the line
   * @param name name of OpAmp
   * @param circuit reference to Circuit being created
   * @return * void
   */
  void ParseOpAmp(std::istringstream& iss, const std::string& name,
                  Circuit& circuit);
};

#endif  // NETLIST_PARSER_HPP
