/**
 * @file component.hpp
 * @brief Defines the Component struct for circuit elements in the GUI.
 *
 * The Component struct represents a single circuit element, including its type,
 * endpoints, and parameters. Used by GridWidget and other GUI classes.
 */
#ifndef GUI_COMPONENT_HPP
#define GUI_COMPONENT_HPP
#include <QMap>
#include <QPoint>
#include <QString>
#include <QVariant>
#include <QVector>

/**
 * @struct Component
 * @brief Represents a circuit component in the GUI.
 *
 * Contains the type of the component (e.g., "Resistor"), its start and end
 * points on the grid, and a map of parameters (such as resistance, capacitance,
 * etc.).
 */
struct GuiComponent {
  /** @brief The type of the component (e.g., "Resistor", "Wire"). */
  QString type;
  /** @brief The starting point of the component on the grid. */
  QPoint startPoint;
  /** @brief The ending point of the component on the grid. */
  QPoint endPoint;
  /** @brief Parameters for the component (name-value pairs). */
  QMap<QString, QVariant> parameters;

  /**
   * @brief Get all connection points (terminals) for this component.
   *
   * For 2-terminal components, returns {startPoint, endPoint}.
   * For Op-Amp, returns {non-inv(+), inv(-), output}.
   * For Dependent Sources, returns {out+, out-, control+, control-}.
   *
   * IMPORTANT: Netlist format for dependent sources is:
   *   Ename n+ n- nc+ nc- val
   * where n+/n- are OUTPUT terminals and nc+/nc- are CONTROL terminals
   */
  QVector<QPoint> getTerminals() const {
    if (type == "VCCS" || type == "CCVS" || type == "CCCS" || type == "VCVS") {
      // All dependent sources
      // GridWidget visual: Left side = Control (A, B), Right side = Output (V+,
      // V-) startPoint: Control+ (Top-Left, "A") endPoint: Control-
      // (Bottom-Left, "B") Netlist format: Xname n+ n- nc+ nc- val where n+/n-
      // are OUTPUT terminals, nc+/nc- are CONTROL terminals Return: [Out+,
      // Out-, Control+, Control-] = [V+, V-, A, B]
      QPoint controlPlus = startPoint;  // A
      QPoint controlMinus = endPoint;   // B
      QPoint outPlus(startPoint.x() + 96,
                     startPoint.y());  // V+ (6 * GRID_SIZE = 96)
      QPoint outMinus(endPoint.x() + 96,
                      endPoint.y());  // V- (6 * GRID_SIZE = 96)
      return {outPlus, outMinus, controlPlus, controlMinus};
    }

    QVector<QPoint> points;
    points.append(startPoint);
    if (type != "GND") {
      points.append(endPoint);
    }

    if (type == "Op-Amp") {
      // Fixed orientation: Triangle pointing right.
      // startPoint: Non-inverting input (+) (Bottom-Left)
      // endPoint: Inverting input (-) (Top-Left)
      // Output: Right tip
      QPoint midBase((startPoint.x() + endPoint.x()) / 2,
                     (startPoint.y() + endPoint.y()) / 2);
      // Output is 80px to the right of the input line center
      QPoint output(midBase.x() + 80, midBase.y());
      points.append(output);
    }

    return points;
  }
};
#endif  // GUI_COMPONENT_HPP