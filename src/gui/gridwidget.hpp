/**
 * @file gridwidget.hpp
 * @brief Widget for drawing and editing circuit components on a grid.
 *
 * Declares the GridWidget class, which provides interactive editing,
 * selection, and visualization of circuit components in the GUI.
 */

#ifndef GRIDWIDGET_HPP
#define GRIDWIDGET_HPP

#include <QHash>
#include <QPoint>
#include <QSet>
#include <QVector>
#include <QWidget>
#include <QtGlobal>
#include <complex>

#include "component.hpp"

using dcomplex = std::complex<double>;

// Provide qHash for QPoint when using it as a key in QHash
inline uint qHash(const QPoint& key, uint seed = 0) noexcept {
  quint64 packed = (static_cast<quint64>(static_cast<quint32>(key.x())) << 32) |
                   static_cast<quint32>(key.y());
  return ::qHash(packed, seed);
}

/**
 * @brief The size of a grid cell in pixels.
 */
const int GRID_SIZE = 16;

/**
 * @brief The size of the grid in points (e.g., 100x100 points).
 */
const int GRID_POINT_COUNT = 100;

/**
 * @class GridWidget
 * @brief Interactive grid for circuit component placement and editing.
 *
 * GridWidget provides a canvas for drawing, selecting, and editing circuit
 * components. It supports mouse and keyboard interaction, panning, zooming,
 * and emits signals when the component type is cleared.
 */
class GridWidget : public QWidget {
  Q_OBJECT
 public:
  /**
   * @brief Construct a new GridWidget.
   * @param parent Optional parent widget (default: nullptr).
   */
  explicit GridWidget(QWidget* parent = nullptr);

  /**
   * @brief Set the current component type for drawing.
   * @param type The name of the component type to draw (e.g., "Resistor").
   */
  void setCurrentComponentType(const QString& type);

  /**
   * @brief Cancel the current selection (deselects any selected component).
   */
  void cancelSelection();

  /**
   * @brief Get the list of components currently in the grid.
   * @return QVector<Component> List of all components.
   */
  QVector<GuiComponent> getComponents() const;

  /**
   * @brief Replace the current component list with a new one.
   * @param comps New set of components to display.
   */
  void setComponents(const QVector<GuiComponent>& comps);

  /**
   * @brief Set per-node voltages to display as tooltips when hovering nodes.
   * @param map Mapping from grid point (node position) to voltage value.
   */
  void setNodeVoltages(const QHash<QPoint, double>& map);

  void setComplexNodeVoltages(const QHash<QPoint, dcomplex>& map);

  /**
   * @brief Set per-component currents to display as tooltips when hovering
   * bodies. The key is the index in `components`.
   */
  void setComponentCurrents(const QHash<int, double>& map);

  void setComponentComplexCurrents(const QHash<int, dcomplex>& map);

  /**
   * @brief Load components from a Falstad-style circuit file content.
   * @param content The file content string.
   */
  void loadFromFalstad(const QString& content);

  /**
   * @brief Set AC to true to show sine wave plots on component right click
   *
   * @param freq AC Frequency
   */
  void setSimType(bool type, double freq = 0.0);

 signals:
  /**
   * @brief Emitted when the current component type is cleared.
   */
  void componentTypeCleared();

 protected:
  /** @brief Paint the grid and components. */
  void paintEvent(QPaintEvent* event) override;
  /** @brief Handle mouse press events for drawing/selecting. */
  void mousePressEvent(QMouseEvent* event) override;
  /** @brief Handle mouse move events for drawing/panning. */
  void mouseMoveEvent(QMouseEvent* event) override;
  /** @brief Handle mouse release events for finishing actions. */
  void mouseReleaseEvent(QMouseEvent* event) override;
  /** @brief Handle double-click events for editing components. */
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  /** @brief Handle mouse wheel events for zooming. */
  void wheelEvent(QWheelEvent* event) override;
  /** @brief Handle key press events for deletion and escape. */
  void keyPressEvent(QKeyEvent* event) override;
  /** @brief Handle key release events. */
  void keyReleaseEvent(QKeyEvent* event) override;

 private:
  /**
   * @brief Snap a position to the nearest grid point.
   * @param pos The position to snap.
   * @return QPoint The snapped grid point.
   */
  QPoint snapToGrid(const QPointF& pos);

  /**
   * @brief Handle type for component manipulation (body, endpoints).
   */
  enum class Handle { None, Body, Start, End };

  /**
   * @brief Result of a hit test (component index and handle type).
   */
  struct HitResult {
    int componentIndex = -1;  ///< Index of the hit component, or -1 if none.
    Handle handle = Handle::None;  ///< Which part of the component was hit.
  };

  /**
   * @brief Perform a hit test to find which component/handle is under a
   * position.
   * @param pos The position to test.
   * @return HitResult Information about the hit.
   */
  HitResult hitTest(const QPointF& pos);

  /** @brief The currently selected component type for drawing. */
  QString currentComponentType;
  /** @brief Start point for drawing a new component. */
  QPoint startPoint;
  /** @brief End point for drawing a new component. */
  QPoint endPoint;
  /** @brief True if currently drawing a component. */
  bool drawing;
  /** @brief List of all components in the grid. */
  QVector<GuiComponent> components;

  /** @brief Start point for panning. */
  QPoint panStartPoint;
  /** @brief True if currently panning the view. */
  bool panning;
  /** @brief Current pan offset. */
  QPointF panOffset;
  /** @brief Current zoom scale. */
  qreal scale;

  /** @brief Index of the cucomponentdialogrrently selected component. */
  int selectedComponentIndex;
  /** @brief Which handle is being moved (if any). */
  Handle movingHandle;
  /** @brief Start point for moving a handle. */
  QPointF moveStartPoint;
  /** @brief Original start point before moving. */
  QPoint originalMoveStartPoint;
  /** @brief Original end point before moving. */
  QPoint originalMoveEndPoint;

  /** @brief Map of grid points to simulated node voltages. */
  QHash<QPoint, double> nodeVoltages;
  /** @brief Mapf of grid points to complex node voltages. */
  QHash<QPoint, dcomplex> nodeComplexVoltages;
  /** @brief Track last tooltip point to reduce flicker. */
  QPoint lastTooltipPoint;
  /** @brief Map of component index to current value (A). */
  QHash<int, double> componentCurrents;
  /** @brief Map of component index to complex current value. */
  QHash<int, dcomplex> componentComplexCurrents;
  /** @brief Last component index shown in tooltip. */
  int lastTooltipComponent = -1;
  /** @brief Used for displaying sine wave plots. */
  bool AC = false;
  /** @brief AC frequency */
  double ACFreq = 0.0;

  /** @brief Set of currently pressed keys. */
  QSet<int> pressedKeys;

  /** @brief Current mouse position in grid coordinates. */
  QPoint currentMouseGridPos;
};

#endif  // GRIDWIDGET_HPP
