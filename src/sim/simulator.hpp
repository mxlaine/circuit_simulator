#pragma once

#include <Eigen/Dense>
#include <complex>
#include <stdexcept>
#include <vector>

class Circuit;
class Node;
class Component;
class ShortCircuit;

using namespace Eigen;
using dcomplex = std::complex<double>;

/**
 * @class Simulator
 * @brief Solves electrical circuits using nodal analysis.
 *
 * Analyzes circuits in DC and AC (steady-state, complex phasor) modes
 * by building and solving systems of linear equations.
 */
class Simulator {
 public:
  /**
   * @brief Default constructor for Simulator.
   */
  Simulator();

  /**
   * @brief Constructs a Simulator with a circuit.
   * @param circuit Pointer to the Circuit object to analyze.
   */
  Simulator(Circuit* circuit);

  /**
   * @brief Solves for DC operating point.
   */
  void SolveDC();

  /**
   * @brief Solves for AC response at a specific frequency.
   * @param frequency Frequency in Hz.
   */
  void SolveAC(double frequency);

  /**
   * @brief The circuit to be simulated
   *
   */
  Circuit* circuit_;

  /**
   * @name Matrix Building
   * @{
   */

  /**
   * @brief Builds conductance matrix for DC analysis.
   * @param m Matrix to fill.
   * @param components Resistive components.
   */
  void FillGCMatrix(MatrixXd& m, std::vector<Component*>& components);

  /**
   * @brief Builds conductance matrix for AC analysis.
   * @param m Complex conductance matrix.
   * @param resistors Resistive components.
   * @param capacitors Capacitive components.
   * @param inductors Inductive components.
   * @param omega Angular frequency (2π × frequency).
   */
  void FillGCMatrixAC(MatrixXcd& m, std::vector<Component*>& resistors,
                      std::vector<Component*>& capacitors,
                      std::vector<Component*>& inductors, double omega);

  /**
   * @brief Builds B matrix for voltage and current sources.
   * @param B Matrix to fill.
   */
  void FillBMatrix(MatrixXd& B);

  /**
   * @brief Builds complete A matrix (G matrix with voltage source rows).
   * @param A Matrix to fill.
   */
  void FillAMatrix(MatrixXd& A);

  /**
   * @name Controlled Sources
   * @{
   */

  /**
   * @brief Adds VCCS (voltage-controlled current source) effects to G matrix.
   * @param G Conductance matrix.
   */
  void AddVCCSToG(MatrixXd& G);

  /**
   * @brief Gets the branch index for a voltage source.
   * @param sc Pointer to the ShortCircuit (voltage source).
   * @return The branch index.
   */
  int GetBranchIndexForShortCircuit(ShortCircuit* sc);

  /**
   * @name Helper Methods
   * @{
   */

  /**
   * @brief Finds the matrix row/column index for a node.
   * @param node Pointer to the Node.
   * @return Node index, or -1 if not found.
   */
  int FindNodeIndex(Node* node);

  /**
   * @brief Validates the circuit for consistency and checks whether circuit is
   * computable.
   * @param isAC True for AC analysis, false for DC.
   * @throw std::runtime_error if circuit is invalid.
   */
  void CheckCircuitTopology(bool isAC);
};
