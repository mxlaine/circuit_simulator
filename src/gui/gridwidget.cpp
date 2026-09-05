#include "gridwidget.hpp"

#include <qcustomplot.h>

#include <QKeyEvent>
#include <QLineF>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QToolTip>
#include <QWheelEvent>
#include <cmath>
#include <numeric>

#include "componentdialog.hpp"
#include "plotdialog.hpp"

GridWidget::GridWidget(QWidget *parent)
    : QWidget(parent), drawing(false), panning(false), scale(1.2),
      selectedComponentIndex(-1), movingHandle(Handle::None),
      currentMouseGridPos(0, 0) {
  setAttribute(Qt::WA_StaticContents);
  setMouseTracking(true);
  setFocusPolicy(Qt::StrongFocus);
}

void GridWidget::setCurrentComponentType(const QString &type) {
  currentComponentType = type;
  selectedComponentIndex = -1;
  movingHandle = Handle::None;
  update();
}

void GridWidget::cancelSelection() {
  selectedComponentIndex = -1;
  update();
}

QVector<GuiComponent> GridWidget::getComponents() const { return components; }

void GridWidget::setComponents(const QVector<GuiComponent> &comps) {
  components = comps;
  selectedComponentIndex = -1;
  movingHandle = Handle::None;
  update();
}

void GridWidget::setNodeVoltages(const QHash<QPoint, double> &map) {
  nodeVoltages = map;
  lastTooltipPoint = QPoint(-1, -1);
  update();
}

void GridWidget::setComplexNodeVoltages(const QHash<QPoint, dcomplex> &map) {
  nodeComplexVoltages = map;
  lastTooltipPoint = QPoint(-1, -1);
  update();
}

void GridWidget::setComponentCurrents(const QHash<int, double> &map) {
  componentCurrents = map;
  lastTooltipComponent = -1;
  update();
}

void GridWidget::setComponentComplexCurrents(const QHash<int, dcomplex> &map) {
  componentComplexCurrents = map;
  lastTooltipComponent = -1;
  update();
}

void GridWidget::setSimType(bool type, double freq) {
  AC = type;
  ACFreq = freq;
}

void GridWidget::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  // Clear background
  QPainter painter(this);
  painter.fillRect(rect(), Qt::black);

  // Build connectivity for error checking
  QHash<QPoint, int> nodeMap;
  int nodeCounter = 0;
  for (const auto &comp : components) {
    for (const auto &pt : comp.getTerminals()) {
      if (!nodeMap.contains(pt)) {
        nodeMap[pt] = nodeCounter++;
      }
    }
  }

  std::vector<int> parent(nodeCounter);
  std::iota(parent.begin(), parent.end(), 0);

  auto find = [&](int i) {
    while (i != parent[i]) {
      parent[i] = parent[parent[i]];
      i = parent[i];
    }
    return i;
  };

  auto unite = [&](int i, int j) {
    int rootI = find(i);
    int rootJ = find(j);
    if (rootI != rootJ)
      parent[rootI] = rootJ;
  };

  for (const auto &comp : components) {
    if (comp.type == "Wire") {
      if (nodeMap.contains(comp.startPoint) &&
          nodeMap.contains(comp.endPoint)) {
        unite(nodeMap[comp.startPoint], nodeMap[comp.endPoint]);
      }
    }
  }

  // Apply panning and zooming transforms
  painter.translate(panOffset);
  painter.scale(scale, scale);

  // Paint Grid
  QPen gridPen(Qt::gray);
  gridPen.setWidth(1);
  painter.setPen(gridPen);
  for (int gx = 0; gx < GRID_POINT_COUNT; ++gx) {
    for (int gy = 0; gy < GRID_POINT_COUNT; ++gy) {
      QPoint gridPoint(gx * GRID_SIZE, gy * GRID_SIZE);
      painter.drawPoint(gridPoint);
    }
  }

  // Paint Components
  QPen linePen(Qt::green);
  linePen.setWidth(3);

  const int maxPos = (GRID_POINT_COUNT - 1) * GRID_SIZE;

  for (int i = 0; i < components.size(); ++i) {
    const auto &component = components[i];

    // Check for op-amp wiring errors
    bool isError = false;
    if (component.type == "Op-Amp") {
      QVector<QPoint> terms = component.getTerminals();
      if (terms.size() >= 3) {
        int p_plus = nodeMap.value(terms[0], -1);
        int p_minus = nodeMap.value(terms[1], -1);
        int p_out = nodeMap.value(terms[2], -1);
        if (p_out != -1) {
          if (p_plus != -1 && find(p_plus) == find(p_out))
            isError = true;
          if (p_minus != -1 && find(p_minus) == find(p_out))
            isError = true;
        }
      }
    }

    if (i == selectedComponentIndex) {
      painter.setPen(Qt::blue);
    } else if (isError) {
      painter.setPen(Qt::red);
    } else {
      painter.setPen(linePen);
    }

    if (component.type == "Op-Amp") {
      QPoint p_plus = component.startPoint;
      QPoint p_minus = component.endPoint;

      // p_plus is bottom (y), p_minus is top (y - 2 * GRID_SIZE)

      int x = p_plus.x();
      int y_plus = p_plus.y();
      int y_minus = p_minus.y();
      int y_mid = (y_plus + y_minus) / 2;

      // Triangle coordinates
      // Height 4 * GRID_SIZE centered at y_mid
      int y_tri_top = y_mid - 2 * GRID_SIZE;
      int y_tri_bottom = y_mid + 2 * GRID_SIZE;

      QPoint tri_top_left(x + 16, y_tri_top);
      QPoint tri_bottom_left(x + 16, y_tri_bottom);
      QPoint tri_tip(x + 64, y_mid);

      // Draw Triangle
      painter.drawLine(tri_top_left, tri_bottom_left);
      painter.drawLine(tri_bottom_left, tri_tip);
      painter.drawLine(tri_tip, tri_top_left);

      // Draw Input Wires
      painter.drawLine(p_plus, QPoint(x + 16, y_plus));   // + input wire
      painter.drawLine(p_minus, QPoint(x + 16, y_minus)); // - input wire

      // Draw Output Wire
      QPoint output(x + 80, y_mid);
      painter.drawLine(tri_tip, output);

      // Draw + and - signs centered on the points
      painter.setPen(Qt::white);
      QFontMetrics fm(painter.font());
      QString plus = "+";
      QString minus = "-";
      int plusWidth = fm.horizontalAdvance(plus);
      int plusHeight = fm.height();
      int minusWidth = fm.horizontalAdvance(minus);
      int minusHeight = fm.height();
      painter.drawText(QPoint(x - plusWidth / 2, y_plus + (8 + plusHeight / 2)),
                       plus);
      painter.drawText(
          QPoint(x - minusWidth / 2, y_minus - (8 + minusHeight / 2)), minus);

      // Draw nodes
      QPen pointPen(Qt::blue);
      pointPen.setWidth(4);
      painter.setPen(pointPen);
      painter.drawPoint(p_plus);
      painter.drawPoint(p_minus);
      painter.drawPoint(output);
    } else if (component.type == "VCVS" || component.type == "VCCS" ||
               component.type == "CCVS" || component.type == "CCCS") {
      // Draw square box in the middle with wires
      // startPoint: In+ (Top-Left)
      // endPoint: In- (Bottom-Left)

      QPoint inPlus = component.startPoint;
      QPoint inMinus = component.endPoint;
      QPoint outPlus(inPlus.x() + 6 * GRID_SIZE, inPlus.y());
      QPoint outMinus(inMinus.x() + 6 * GRID_SIZE, inMinus.y());

      // Box coordinates (4x4 grid cells in the middle)
      // Total width is 6 grid cells. Box is cells 3 and 4 (indices 0-5).
      // So box starts at x + 1*GRID_SIZE and ends at x + 5*GRID_SIZE.
      int boxLeftX = inPlus.x() + 1 * GRID_SIZE;
      int boxRightX = inPlus.x() + 5 * GRID_SIZE;
      int boxTopY = inPlus.y() - 1 * GRID_SIZE;
      int boxBottomY = inMinus.y() + 1 * GRID_SIZE;

      if (i == selectedComponentIndex) {
        painter.setPen(Qt::blue);
      } else {
        painter.setPen(linePen);
      }

      // Draw Box
      painter.drawRect(boxLeftX, boxTopY, boxRightX - boxLeftX,
                       boxBottomY - boxTopY);

      // Draw Wires
      painter.drawLine(inPlus, QPoint(boxLeftX, inPlus.y())); // Top-Left wire
      painter.drawLine(inMinus,
                       QPoint(boxLeftX, inMinus.y())); // Bottom-Left wire
      painter.drawLine(QPoint(boxRightX, outPlus.y()),
                       outPlus); // Top-Right wire
      painter.drawLine(QPoint(boxRightX, outMinus.y()),
                       outMinus); // Bottom-Right wire

      // Draw terminals
      QPen pointPen(Qt::blue);
      pointPen.setWidth(4);
      painter.setPen(pointPen);
      painter.drawPoint(outPlus);
      painter.drawPoint(outMinus);
      painter.drawPoint(inPlus);
      painter.drawPoint(inMinus);

      // Draw labels
      painter.setPen(Qt::white);
      // A and B on left inside box
      painter.drawText(QPoint(boxLeftX + 3, boxTopY + 15), "A");
      painter.drawText(QPoint(boxLeftX + 3, boxBottomY - 5), "B");
      // V+ and V- on right inside box
      painter.drawText(QPoint(boxRightX - 20, boxTopY + 15), "V+");
      painter.drawText(QPoint(boxRightX - 20, boxBottomY - 5), "V-");

      // Draw Type and Value centered
      painter.save();
      QString valueStr;
      if (component.type == "VCVS" || component.type == "CCCS") {
        valueStr = component.parameters.value("Gain").toString();
      } else if (component.type == "VCCS") {
        valueStr = component.parameters.value("Transconductance").toString();
      } else if (component.type == "CCVS") {
        valueStr = component.parameters.value("Transresistance").toString();
      }

      QRect boxRect(boxLeftX, boxTopY, boxRightX - boxLeftX,
                    boxBottomY - boxTopY);
      QFont f = painter.font();
      f.setPointSize(7);
      painter.setFont(f);
      painter.drawText(boxRect, Qt::AlignCenter,
                       component.type + "\n" + valueStr);
      painter.restore();
    } else {
      // 2-terminal components
      QPoint midPoint = (component.startPoint + component.endPoint) / 2;
      double length =
          std::hypot(component.endPoint.x() - component.startPoint.x(),
                     component.endPoint.y() - component.startPoint.y());

      painter.save();
      painter.translate(component.startPoint);
      qreal angle = atan2(component.endPoint.y() - component.startPoint.y(),
                          component.endPoint.x() - component.startPoint.x()) *
                    180 / M_PI;
      painter.rotate(angle);

      if (component.type == "Wire") {
        painter.drawLine(0, 0, length, 0);
      } else if (component.type == "Resistor") {
        double p1 = length / 2 - 10;
        double p2 = length / 2 + 10;
        painter.drawLine(0, 0, p1, 0);
        painter.drawRect(p1, -5, 20, 10);
        painter.drawLine(p2, 0, length, 0);
      } else if (component.type == "Capacitor") {
        double p1 = length / 2 - 2;
        double p2 = length / 2 + 2;
        painter.drawLine(0, 0, p1, 0);
        painter.drawLine(p1, -10, p1, 10);
        painter.drawLine(p2, -10, p2, 10);
        painter.drawLine(p2, 0, length, 0);
      } else if (component.type == "Inductor") {
        double p1 = length / 2 - 15;
        double p2 = length / 2 + 15;
        painter.drawLine(0, 0, p1, 0);
        // Draw 3 semicircles
        for (int k = 0; k < 3; ++k) {
          painter.drawArc(p1 + k * 10, -5, 10, 10, 0, 180 * 16);
        }
        painter.drawLine(p2, 0, length, 0);
      } else if (component.type == "Voltage Source") {
        double p1 = length / 2 - 2;
        double p2 = length / 2 + 2;
        painter.drawLine(0, 0, p1, 0);
        // Long line (positive) at p1
        painter.drawLine(p1, -10, p1, 10);
        // Short line (negative) at p2
        painter.drawLine(p2, -5, p2, 5);
        painter.drawLine(p2, 0, length, 0);

        // Draw + sign near positive terminal
        painter.setPen(Qt::white);
        painter.drawText(p1 - 10, -10, "+");
        if (i == selectedComponentIndex)
          painter.setPen(Qt::blue);
        else
          painter.setPen(linePen);

      } else if (component.type == "Current Source") {
        double p1 = length / 2 - 10;
        double p2 = length / 2 + 10;
        painter.drawLine(0, 0, p1, 0);
        painter.drawEllipse(QPointF(length / 2, 0), 10, 10);
        // Arrow inside
        painter.drawLine(length / 2 - 5, 0, length / 2 + 5, 0);
        painter.drawLine(length / 2 + 5, 0, length / 2 + 2, -3);
        painter.drawLine(length / 2 + 5, 0, length / 2 + 2, 3);
        painter.drawLine(p2, 0, length, 0);
      } else if (component.type == "GND") {
        painter.drawLine(0, 0, length, 0);

        painter.drawLine(length, -10, length, 10);
        painter.drawLine(length + 4, -6, length + 4, 6);
        painter.drawLine(length + 8, -2, length + 8, 2);
      } else {
        painter.drawLine(0, 0, length, 0);
      }

      painter.restore();

      QPen pointPen(Qt::blue);
      pointPen.setWidth(4);
      painter.setPen(pointPen);
      painter.drawPoint(component.startPoint);
      if (component.type != "GND") {
        painter.drawPoint(component.endPoint);
      }
    }

    QPoint midPoint = (component.startPoint + component.endPoint) / 2;

    painter.save();
    painter.translate(midPoint);
    qreal angle = atan2(component.endPoint.y() - component.startPoint.y(),
                        component.endPoint.x() - component.startPoint.x()) *
                  180 / M_PI;
    if (angle > 90)
      angle -= 180;
    if (angle < -90)
      angle += 180;
    painter.rotate(angle);

    painter.setPen(Qt::white);

    QFontMetrics fm(painter.font());

    QString paramsString;
    if (component.type != "VCVS" && component.type != "VCCS" &&
        component.type != "CCVS" && component.type != "CCCS" &&
        component.type != "Op-Amp") {
      for (auto it = component.parameters.constBegin();
           it != component.parameters.constEnd(); ++it) {
        paramsString += it.value().toString() + " ";
      }
      if (!paramsString.isEmpty()) {
        paramsString.chop(1);
        int paramsWidth = fm.horizontalAdvance(paramsString);
        QPoint paramsPoint(-paramsWidth / 2, -16);
        painter.drawText(paramsPoint, paramsString);
      }
    }

    painter.restore();
  }

  // Paint currently drawing component
  if (drawing) {
    painter.setPen(linePen);
    painter.drawLine(startPoint, endPoint);
  }

  // Draw cursor coordinates
  painter.resetTransform();
  QString coordText = QString("X: %1, Y: %2")
                          .arg(currentMouseGridPos.x())
                          .arg(currentMouseGridPos.y());
  painter.setPen(Qt::white);
  painter.drawText(rect().adjusted(0, 0, -5, -5),
                   Qt::AlignRight | Qt::AlignBottom, coordText);
}

void GridWidget::mousePressEvent(QMouseEvent *event) {
  // Start panning on middle mouse button
  if (event->button() == Qt::MiddleButton) {
    panning = true;
    panStartPoint = event->pos();

  }
  // Handle left mouse button press
  else if (event->button() == Qt::LeftButton) {
    // Convert mouse position to grid coordinates
    QPointF worldPos = (event->pos() - panOffset) / scale;
    if (movingHandle != Handle::None) {
      movingHandle = Handle::None;
      selectedComponentIndex = -1;
    } else if (!currentComponentType.isEmpty()) {
      startPoint = snapToGrid(worldPos);
      endPoint = startPoint;
      drawing = true;
      selectedComponentIndex = -1;
    } else {
      HitResult hit = hitTest(worldPos);
      if (hit.componentIndex != -1) {
        selectedComponentIndex = hit.componentIndex;
        movingHandle = hit.handle;
        originalMoveStartPoint = components[hit.componentIndex].startPoint;
        originalMoveEndPoint = components[hit.componentIndex].endPoint;
        moveStartPoint = worldPos;
      } else {
        selectedComponentIndex = -1;
      }
    }
    update();
  } else if (event->button() == Qt::RightButton) {
    QPointF worldPos = (event->pos() - panOffset) / scale;

    HitResult hit = hitTest(worldPos);
    if (hit.componentIndex != -1 && hit.handle == Handle::Body &&
        componentComplexCurrents.contains(hit.componentIndex) && AC) {
      dcomplex i = componentComplexCurrents.value(hit.componentIndex);
      dcomplex v = 0.0;
      const auto &comp = components[hit.componentIndex];
      if (nodeComplexVoltages.contains(comp.startPoint) &&
          nodeComplexVoltages.contains(comp.endPoint)) {
        v = nodeComplexVoltages.value(comp.startPoint) -
            nodeComplexVoltages.value(comp.endPoint);
      }
      PlotDialog *dlg = new PlotDialog(this, i, v, ACFreq);
      dlg->setAttribute(Qt::WA_DeleteOnClose);
      dlg->exec();
    }
  }
}

void GridWidget::mouseMoveEvent(QMouseEvent *event) {
  QPointF worldPosRaw = (event->pos() - panOffset) / scale;
  QPoint newGridPos = snapToGrid(worldPosRaw);
  if (newGridPos != currentMouseGridPos) {
    currentMouseGridPos = newGridPos;
    update();
  }

  if (panning) {
    panOffset += event->pos() - panStartPoint;
    panStartPoint = event->pos();
    // Clamp panOffset so grid stays in view
    const int gridPixelSize = (GRID_POINT_COUNT - 1) * GRID_SIZE;
    qreal maxPanX = 0;
    qreal minPanX = width() - gridPixelSize * scale;
    qreal maxPanY = 0;
    qreal minPanY = height() - gridPixelSize * scale;
    panOffset.setX(std::min(maxPanX, std::max(panOffset.x(), minPanX)));
    panOffset.setY(std::min(maxPanY, std::max(panOffset.y(), minPanY)));
    update();
  } else if (drawing) {
    QPointF worldPos = (event->pos() - panOffset) / scale;
    QPointF currentPoint = snapToGrid(worldPos);

    if (currentComponentType == "Op-Amp") {
      // Fixed orientation: Vertical inputs, output to right.
      // startPoint: Plus input (Bottom)
      // endPoint: Minus input (Top)
      endPoint.setX(startPoint.x());
      endPoint.setY(startPoint.y() - 2 * GRID_SIZE);
    } else if (currentComponentType == "VCVS" ||
               currentComponentType == "VCCS" ||
               currentComponentType == "CCVS" ||
               currentComponentType == "CCCS") {
      // Fixed orientation: Square box.
      // startPoint: In+ (Top-Left)
      // endPoint: In- (Bottom-Left)
      endPoint.setX(startPoint.x());
      endPoint.setY(startPoint.y() + 2 * GRID_SIZE);
    } else {
      int dx = std::abs(currentPoint.x() - startPoint.x());
      int dy = std::abs(currentPoint.y() - startPoint.y());

      if (dx > dy) {
        endPoint = QPoint(currentPoint.x(), startPoint.y());
      } else {
        endPoint = QPoint(startPoint.x(), currentPoint.y());
      }
    }
    update();
  } else if (movingHandle != Handle::None && selectedComponentIndex != -1) {
    QPointF worldPos = (event->pos() - panOffset) / scale;
    QPointF delta = worldPos - moveStartPoint;

    if (movingHandle == Handle::Body) {
      QPoint newStart = originalMoveStartPoint + delta.toPoint();
      QPoint newEnd = originalMoveEndPoint + delta.toPoint();
      components[selectedComponentIndex].startPoint = snapToGrid(newStart);
      components[selectedComponentIndex].endPoint = snapToGrid(newEnd);
    } else if (movingHandle == Handle::Start) {
      QPoint newStart = originalMoveStartPoint + delta.toPoint();
      components[selectedComponentIndex].startPoint = snapToGrid(newStart);
    } else if (movingHandle == Handle::End) {
      QPoint newEnd = originalMoveEndPoint + delta.toPoint();
      components[selectedComponentIndex].endPoint = snapToGrid(newEnd);
    }

    update();
  }
  // Show tooltip with node voltage when hovering near a node
  else {
    QPointF worldPos = (event->pos() - panOffset) / scale;
    QPoint gp = snapToGrid(worldPos);
    // Only show tooltip when close to the grid point
    const qreal maxDist = 0.45 * GRID_SIZE;
    if (QLineF(worldPos, gp).length() <= maxDist && nodeVoltages.contains(gp)) {
      if (gp != lastTooltipPoint) {
        if (AC) {
          dcomplex v = nodeComplexVoltages.value(gp);
          double v_rms = std::abs(v);
          double v_phase = std::arg(v);
          QToolTip::showText(QCursor::pos(),
                             QString("V = %1 V (RMS), arg: %2 rad")
                                 .arg(v_rms, 0, 'f', 3)
                                 .arg(v_phase, 0, 'f', 2),
                             this);
        } else {
          double v = nodeVoltages.value(gp);
          QToolTip::showText(QCursor::pos(),
                             QString("V = %1 V").arg(v, 0, 'f', 3), this);
        }
        lastTooltipPoint = gp;
        lastTooltipComponent = -1;
      }
    } else {
      // If not near a node, check if hovering over a component body and show
      // current
      HitResult hit = hitTest(worldPos);
      if (hit.componentIndex != -1 && hit.handle == Handle::Body &&
          componentCurrents.contains(hit.componentIndex)) {
        if (hit.componentIndex != lastTooltipComponent) {
          const auto &comp = components[hit.componentIndex];
          if (AC) {
            dcomplex i = componentComplexCurrents.value(hit.componentIndex);
            double i_rms = std::abs(i);
            double i_phase = std::arg(i);
            dcomplex v = {0.0, 0.0};
            if (nodeComplexVoltages.contains(comp.startPoint) &&
                nodeComplexVoltages.contains(comp.endPoint)) {
              v = nodeComplexVoltages.value(comp.startPoint) -
                  nodeComplexVoltages.value(comp.endPoint);
            }
            double v_rms = std::abs(v);
            double v_phase = std::arg(v);
            QString tooltip =
                QString(
                    "I = %1 A (RMS), arg: %2 rad\nΔV = %3 V (RMS), arg: %4 rad"
                    "\nRight-click to open plot")
                    .arg(i_rms, 0, 'f', 6)
                    .arg(i_phase, 0, 'f', 2)
                    .arg(v_rms, 0, 'f', 3)
                    .arg(v_phase, 0, 'f', 2);
            QToolTip::showText(QCursor::pos(), tooltip, this);

          } else {
            double i = componentCurrents.value(hit.componentIndex);
            double vdiff = 0.0;
            if (nodeVoltages.contains(comp.startPoint) &&
                nodeVoltages.contains(comp.endPoint)) {
              vdiff = nodeVoltages.value(comp.startPoint) -
                      nodeVoltages.value(comp.endPoint);
            }
            QString tooltip = QString("I = %1 A\nΔV = %2 V")
                                  .arg(i, 0, 'f', 6)
                                  .arg(vdiff, 0, 'f', 3);
            QToolTip::showText(QCursor::pos(), tooltip, this);
          }
          lastTooltipComponent = hit.componentIndex;
          lastTooltipPoint = QPoint(-1, -1);
        }
      } else {
        if (lastTooltipPoint != QPoint(-1, -1) || lastTooltipComponent != -1) {
          QToolTip::hideText();
          lastTooltipPoint = QPoint(-1, -1);
          lastTooltipComponent = -1;
        }
      }
    }
  }
}

void GridWidget::mouseReleaseEvent(QMouseEvent *event) {
  // Handle middle mouse button release
  if (event->button() == Qt::MiddleButton) {
    panning = false;
  }
  // Handle left mouse button release
  else if (event->button() == Qt::LeftButton) {
    if (drawing) {
      QPointF worldPos = (event->pos() - panOffset) / scale;
      QPointF currentPoint = snapToGrid(worldPos);

      if (currentComponentType == "Op-Amp") {
        // Fixed orientation: Vertical inputs, output to right.
        // startPoint: Plus input (Bottom)
        // endPoint: Minus input (Top)
        endPoint.setX(startPoint.x());
        endPoint.setY(startPoint.y() - 2 * GRID_SIZE);
      } else if (currentComponentType == "VCVS" ||
                 currentComponentType == "VCCS" ||
                 currentComponentType == "CCVS" ||
                 currentComponentType == "CCCS") {
        // Fixed orientation: Square box.
        // startPoint: In+ (Top-Left)
        // endPoint: In- (Bottom-Left)
        endPoint.setX(startPoint.x());
        endPoint.setY(startPoint.y() + 2 * GRID_SIZE);
      } else {
        int dx = std::abs(currentPoint.x() - startPoint.x());
        int dy = std::abs(currentPoint.y() - startPoint.y());

        if (dx > dy) {
          endPoint = QPoint(currentPoint.x(), startPoint.y());
        } else {
          endPoint = QPoint(startPoint.x(), currentPoint.y());
        }
      }

      if (startPoint != endPoint) {
        GuiComponent newComponent;
        newComponent.type = currentComponentType;
        newComponent.startPoint = startPoint;
        newComponent.endPoint = endPoint;
        if (newComponent.type == "Resistor") {
          newComponent.parameters["Resistance"] = "1000";
        } else if (newComponent.type == "Capacitor") {
          newComponent.parameters["Capacitance"] = "10";
        } else if (newComponent.type == "Inductor") {
          newComponent.parameters["Inductance"] = "10";
        } else if (newComponent.type == "Voltage Source") {
          newComponent.parameters["Voltage"] = "5";
        } else if (newComponent.type == "Current Source") {
          newComponent.parameters["Current"] = "1";
        } else if (newComponent.type == "Op-Amp") {
          newComponent.parameters["Gain"] = "10";
        } else if (newComponent.type == "VCVS") {
          newComponent.parameters["Gain"] = "1";
        } else if (newComponent.type == "VCCS") {
          newComponent.parameters["Transconductance"] = "1";
        } else if (newComponent.type == "CCVS") {
          newComponent.parameters["Transresistance"] = "1";
        } else if (newComponent.type == "CCCS") {
          newComponent.parameters["Gain"] = "1";
        }
        components.append(newComponent);
      }
      drawing = false;
      update();
    }
  }
}

void GridWidget::mouseDoubleClickEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    QPointF worldPos = (event->pos() - panOffset) / scale;
    HitResult hit = hitTest(worldPos);
    if (hit.componentIndex != -1) {
      ComponentDialog dialog(components[hit.componentIndex], this);
      if (dialog.exec() == QDialog::Accepted) {
        components[hit.componentIndex].type = dialog.componentType();
        components[hit.componentIndex].parameters = dialog.parameters();
        selectedComponentIndex = -1;
        movingHandle = Handle::None;
        update();
      }
    }
  }
}

void GridWidget::wheelEvent(QWheelEvent *event) {
  qreal oldScale = scale;
  const int gridPixelSize = (GRID_POINT_COUNT - 1) * GRID_SIZE;

  // Minimum scale so grid fits entirely in the widget
  qreal minScale =
      std::max((qreal)width() / gridPixelSize, (qreal)height() / gridPixelSize);
  // Maximum scale
  qreal maxScale = 5;

  // Update scale based on mouse wheel input
  if (event->angleDelta().y() > 0) {
    scale *= 1.1;
  } else {
    scale /= 1.1;
  }
  // Clamp scale so grid is always visible (prevents zooming out too far)
  scale = std::max(minScale, std::min(scale, maxScale));

  QPointF mousePos = event->position();
  // Adjust panOffset so zoom centers on mouse position
  panOffset = mousePos - (mousePos - panOffset) * (scale / oldScale);

  // Clamp panOffset so grid stays in view (prevents panning out of bounds)
  qreal maxPanX = 0;
  qreal minPanX = width() - gridPixelSize * scale;
  qreal maxPanY = 0;
  qreal minPanY = height() - gridPixelSize * scale;
  panOffset.setX(std::min(maxPanX, std::max(panOffset.x(), minPanX)));
  panOffset.setY(std::min(maxPanY, std::max(panOffset.y(), minPanY)));

  update();
}

void GridWidget::keyPressEvent(QKeyEvent *event) {
  pressedKeys.insert(event->key());

  // Check for Ctrl + A + Delete to clear all components
  bool ctrl = (event->modifiers() & Qt::ControlModifier) ||
              pressedKeys.contains(Qt::Key_Control);
  bool a = pressedKeys.contains(Qt::Key_A);
  bool del = pressedKeys.contains(Qt::Key_Delete);

  if (ctrl && a && del) {
    components.clear();
    selectedComponentIndex = -1;
    movingHandle = Handle::None;
    update();
    return;
  }

  if (event->key() == Qt::Key_Delete && selectedComponentIndex != -1) {
    components.remove(selectedComponentIndex);
    selectedComponentIndex = -1;
    movingHandle = Handle::None;
    update();
  } else if (event->key() == Qt::Key_Escape) {
    if (movingHandle != Handle::None) {
      components[selectedComponentIndex].startPoint = originalMoveStartPoint;
      components[selectedComponentIndex].endPoint = originalMoveEndPoint;
      movingHandle = Handle::None;
      selectedComponentIndex = -1;
      update();
    } else if (!currentComponentType.isEmpty()) {
      currentComponentType.clear();
      emit componentTypeCleared();
      update();
    }
  }
}

void GridWidget::keyReleaseEvent(QKeyEvent *event) {
  pressedKeys.remove(event->key());
  QWidget::keyReleaseEvent(event);
}

QPoint GridWidget::snapToGrid(const QPointF &pos) {
  int x = round(pos.x() / (double)GRID_SIZE) * GRID_SIZE;
  int y = round(pos.y() / (double)GRID_SIZE) * GRID_SIZE;
  const int maxPos = (GRID_POINT_COUNT - 1) * GRID_SIZE;
  x = std::max(0, std::min(x, maxPos));
  y = std::max(0, std::min(y, maxPos));
  return QPoint(x, y);
}

GridWidget::HitResult GridWidget::hitTest(const QPointF &pos) {
  const qreal handleRadius = 10.0 / scale;
  const qreal selectionThreshold = 10.0 / scale;

  // Check components in reverse order for topmost hit
  for (int i = components.size() - 1; i >= 0; --i) {
    const auto &comp = components[i];

    bool fixedSize =
        (comp.type == "Op-Amp" || comp.type == "VCVS" || comp.type == "VCCS" ||
         comp.type == "CCVS" || comp.type == "CCCS");

    if (!fixedSize) {
      if (QLineF(pos, comp.startPoint).length() < handleRadius) {
        return {i, Handle::Start};
      }
      if (QLineF(pos, comp.endPoint).length() < handleRadius) {
        return {i, Handle::End};
      }
    }

    QLineF line(comp.startPoint, comp.endPoint);
    QPointF p1 = line.p1();
    QPointF p2 = line.p2();
    QPointF p = pos;

    qreal lineLengthSq = line.length() * line.length();
    if (lineLengthSq == 0.0) {
      if (QLineF(p, p1).length() < selectionThreshold) {
        return {i, Handle::Body};
      }
    } else {
      qreal t =
          std::max(0.0, std::min(1.0, QPointF::dotProduct(p - p1, p2 - p1) /
                                          lineLengthSq));
      QPointF projection = p1 + t * (p2 - p1);
      if (QLineF(p, projection).length() < selectionThreshold) {
        return {i, Handle::Body};
      }
    }
  }
  return {-1, Handle::None};
}

void GridWidget::loadFromFalstad(const QString &content) {
  components.clear();
  QStringList lines = content.split('\n');

  for (const QString &line : lines) {
    if (line.isEmpty() || line.startsWith('$'))
      continue;

    QStringList parts = line.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 5)
      continue;

    QString typeChar = parts[0];

    int x1 = parts[1].toInt();
    int y1 = parts[2].toInt();
    int x2 = parts[3].toInt();
    int y2 = parts[4].toInt();

    GuiComponent comp;
    comp.startPoint = QPoint(x1, y1);
    comp.endPoint = QPoint(x2, y2);

    if (typeChar == "r") {
      comp.type = "Resistor";
      if (parts.size() > 6)
        comp.parameters["Resistance"] = parts[6].toDouble();
    } else if (typeChar == "212" || typeChar == "213" || typeChar == "214" ||
               typeChar == "215") {
      // Extract numeric value from expression in parts[7]
      // Format: "value*(a-b)" where we want to extract 'value'
      double paramValue = 1.0;
      if (parts.size() > 7) {
        QString expr = parts[7];
        // Extract the leading numeric part
        int i = 0;
        while (i < expr.length() &&
               (expr[i].isDigit() || expr[i] == '.' || expr[i] == '-' ||
                expr[i] == '+' || expr[i] == 'e' || expr[i] == 'E')) {
          i++;
        }
        if (i > 0) {
          paramValue = expr.left(i).toDouble();
        }
      }

      if (typeChar == "212") {
        comp.type = "VCVS";
        comp.parameters["Gain"] = paramValue;
      } else if (typeChar == "213") {
        comp.type = "VCCS";
        comp.parameters["Transconductance"] = paramValue;
      } else if (typeChar == "214") {
        comp.type = "CCVS";
        comp.parameters["Transresistance"] = paramValue;
      } else if (typeChar == "215") {
        comp.type = "CCCS";
        comp.parameters["Gain"] = paramValue;
      }

      // All dependent sources use the same format in our
      // generateFalstadContent: typeChar ox1 oy1 cx1 cy1 ... where x1,y1 =
      // OUTPUT+ and x2,y2 = CONTROL+
      //
      // In GUI:
      // - startPoint = CONTROL+ (left side, top)
      // - endPoint = CONTROL- (left side, bottom)
      // - OUTPUT terminals are at startPoint/endPoint + 6*GRID_SIZE to the
      // right

      comp.startPoint = snapToGrid(QPointF(x2, y2));
      comp.endPoint = snapToGrid(QPointF(x2, y2 + 2 * GRID_SIZE));

    } else if (typeChar == "w") {
      comp.type = "Wire";
    } else if (typeChar == "v") {
      comp.type = "Voltage Source";
      // Falstad v: v x1 y1 x2 y2 flags 0 0 voltage ...
      // Voltage is at index 8
      if (parts.size() > 8)
        comp.parameters["Voltage"] = parts[8].toDouble();
    } else if (typeChar == "c") {
      comp.type = "Capacitor";
      if (parts.size() > 6)
        comp.parameters["Capacitance"] = parts[6].toDouble();
    } else if (typeChar == "l") {
      comp.type = "Inductor";
      if (parts.size() > 6)
        comp.parameters["Inductance"] = parts[6].toDouble();
    } else if (typeChar == "i") {
      comp.type = "Current Source";
      if (parts.size() > 6)
        comp.parameters["Current"] = parts[6].toDouble();
    } else if (typeChar == "g") {
      comp.type = "GND";
      // GND only needs startPoint for connection
      // endPoint defines the visual representation
    } else if (typeChar == "a") {
      comp.type = "Op-Amp";
      // Falstad a: a x1 y1 x2 y2 flags max min gain rail+ rail-
      if (parts.size() > 8)
        comp.parameters["Gain"] = parts[8].toDouble();
      // Adjust coordinates for Op-Amp to match GridWidget's expectation
      // Falstad gives axis points (x1,y1) -> (x2,y2)
      double dx = x2 - x1;
      double dy = y2 - y1;
      double len = std::sqrt(dx * dx + dy * dy);
      if (len > 1e-5) {
        // Perpendicular vector for terminals (rotate -90 degrees)
        double px = dy / len;
        double py = -dx / len;
        int offset = GRID_SIZE;
        comp.startPoint =
            snapToGrid(QPointF(x1 + px * offset, y1 + py * offset));
        comp.endPoint = snapToGrid(QPointF(x1 - px * offset, y1 - py * offset));
      }
    }

    if (!comp.type.isEmpty()) {
      components.append(comp);
    }
  }

  selectedComponentIndex = -1;
  movingHandle = Handle::None;
  update();
}