#include "mainwindow.hpp"

#include <QActionGroup>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QTemporaryFile>
#include <QTextStream>
#include <map>
#include <set>

#include "./ui_mainwindow.h"
#include "gridwidget.hpp"
#include "netlist_parser.hpp"
#include "simulator.hpp"

struct QPointLessThan {
  bool operator()(const QPoint& p1, const QPoint& p2) const {
    if (p1.y() < p2.y()) return true;
    if (p1.y() > p2.y()) return false;
    return p1.x() < p2.x();
  }
};

// Simple disjoint-set union (union-find) helper for merging wired nodes
struct DSU {
  std::vector<int> parent;
  explicit DSU(int n) {
    parent.resize(n);
    for (int i = 0; i < n; ++i) parent[i] = i;
  }
  int find(int i) { return parent[i] == i ? i : (parent[i] = find(parent[i])); }
  void unite(int i, int j) {
    int ri = find(i), rj = find(j);
    if (ri != rj) parent[ri] = rj;
  }
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);

  // Show a popup with quick usage instructions
  QMessageBox::information(
      this, "Quick Start Guide",
      "<b>Welcome to Circuit Simulator!</b><br><br>"
      "<b>Key Bindings:</b><br>"
      "- <b>Left mouse button</b>: Select component / node<br>"
      "- <b>Middle mouse button</b>: Pan<br>"
      "- <b>Double-click</b>: Edit component properties<br>"
      "- <b>Del</b>: Delete selected component<br>"
      "- <b>Ctrl + A + Del</b>: Delete all components<br>"
      "- <b>Esc</b>: Cancel current action<br>"
      "<br>"
      "Use the toolbar to add components.<br>"
      "Click and drag to place them on the grid.<br>"
      "<br>"
      "<b>Run Simulation:</b> Runs the configured simulation (DC or AC) of "
      "your circuit and displays "
      "results directly on the grid, without saving a file.<br>"
      "<b>Configure Simulation:</b> Choose between DC and AC simulation types "
      "and set frequency.<br>"
      "<b>Save As:</b> Saves the current circuit as a netlist file for later "
      "use.<br>"
      "<b>Open File:</b> Loads a circuit from a file (supports "
      "https://www.falstad.com/circuit/ .txt format).");

  gridWidget = new GridWidget(this);
  setCentralWidget(gridWidget);

  // Set up the action group for component selection
  actionGroup = new QActionGroup(this);
  actionGroup->addAction(ui->actionResistor);
  actionGroup->addAction(ui->actionVoltage_Source);
  actionGroup->addAction(ui->actionCurrent_Source);
  actionGroup->addAction(ui->actionInductor);
  actionGroup->addAction(ui->actionCapacitor);
  actionGroup->addAction(ui->actionWire);
  actionGroup->addAction(ui->actionOpAmp);
  actionGroup->addAction(ui->actionVCVS);
  actionGroup->addAction(ui->actionVCCS);
  actionGroup->addAction(ui->actionCCVS);
  actionGroup->addAction(ui->actionCCCS);
  actionGroup->addAction(ui->actionGND);

  actionGroup->setExclusive(false);

  // Connect signals to slots
  connect(ui->actionResistor, &QAction::toggled, this,
          &MainWindow::onActionToggled);
  connect(ui->actionVoltage_Source, &QAction::toggled, this,
          &MainWindow::onActionToggled);
  connect(ui->actionCurrent_Source, &QAction::toggled, this,
          &MainWindow::onActionToggled);
  connect(ui->actionInductor, &QAction::toggled, this,
          &MainWindow::onActionToggled);
  connect(ui->actionCapacitor, &QAction::toggled, this,
          &MainWindow::onActionToggled);
  connect(ui->actionWire, &QAction::toggled, this,
          &MainWindow::onActionToggled);
  connect(ui->actionOpAmp, &QAction::toggled, this,
          &MainWindow::onActionToggled);
  connect(ui->actionVCVS, &QAction::toggled, this,
          &MainWindow::onActionToggled);
  connect(ui->actionVCCS, &QAction::toggled, this,
          &MainWindow::onActionToggled);
  connect(ui->actionCCVS, &QAction::toggled, this,
          &MainWindow::onActionToggled);
  connect(ui->actionCCCS, &QAction::toggled, this,
          &MainWindow::onActionToggled);
  connect(ui->actionGND, &QAction::toggled, this, &MainWindow::onActionToggled);

  connect(gridWidget, &GridWidget::componentTypeCleared, this,
          &MainWindow::onComponentTypeCleared);

  connect(ui->actionSave_As, &QAction::triggered, this, &MainWindow::onSaveAs);
  connect(ui->actionOpen_File, &QAction::triggered, this,
          &MainWindow::onOpenFile);

  // Add Run Simulation action to File menu
  QAction* runSimAction = new QAction(tr("Run Simulation"), this);
  connect(runSimAction, &QAction::triggered, this,
          &MainWindow::onRunSimulation);
  if (ui->menuFile) {
    ui->menuFile->addAction(runSimAction);
  } else if (menuBar()) {
    menuBar()->addAction(runSimAction);
  }

  // Add Configure Simulation action to File menu
  QAction* configSimAction = new QAction(tr("Configure Simulation..."), this);
  connect(configSimAction, &QAction::triggered, this,
          &MainWindow::onConfigureSimulation);
  if (ui->menuFile) {
    ui->menuFile->addAction(configSimAction);
  }

  // Preload a simple circuit and run simulation once
  {
    QVector<GuiComponent> initial;

    // Resistor: top edge
    {
      GuiComponent r;
      r.type = "Resistor";
      r.startPoint = QPoint(96, 96);
      r.endPoint = QPoint(192, 96);
      r.parameters["Resistance"] = "1000";  // 1 kOhm
      initial.append(r);
    }

    // Voltage source: right edge
    {
      GuiComponent v;
      v.type = "Voltage Source";
      v.startPoint = QPoint(192, 96);
      v.endPoint = QPoint(192, 192);
      v.parameters["Voltage"] = "5";  // 5 V
      initial.append(v);
    }

    // Wire: bottom edge
    {
      GuiComponent w;
      w.type = "Wire";
      w.startPoint = QPoint(192, 192);
      w.endPoint = QPoint(96, 192);
      initial.append(w);
    }

    // Wire: left edge
    {
      GuiComponent w;
      w.type = "Wire";
      w.startPoint = QPoint(96, 192);
      w.endPoint = QPoint(96, 96);
      initial.append(w);
    }

    gridWidget->setComponents(initial);
    onRunSimulation();
  }
}

MainWindow::~MainWindow() {
  delete ui;
  delete actionGroup;
  delete gridWidget;
}

void MainWindow::onComponentTypeCleared() {
  for (QAction* action : actionGroup->actions()) {
    action->setChecked(false);
  }
}

void MainWindow::onConfigureSimulation() {
  QStringList items;
  items << tr("DC") << tr("AC");

  bool ok;
  QString item = QInputDialog::getItem(
      this, tr("Simulation Type"), tr("Select Simulation Type:"), items,
      simulationType == SimulationType::DC ? 0 : 1, false, &ok);
  if (ok && !item.isEmpty()) {
    if (item == tr("DC")) {
      simulationType = SimulationType::DC;
      gridWidget->setSimType(false);
    } else {
      simulationType = SimulationType::AC;
      double freq = QInputDialog::getDouble(this, tr("AC Frequency"),
                                            tr("Frequency (Hz):"), acFrequency,
                                            0, 1e9, 2, &ok);
      if (ok) {
        acFrequency = freq;
      }
      gridWidget->setSimType(true, freq);
    }
  }
}

void MainWindow::onRunSimulation() {
  // Generate netlist in-memory (temporary file) and simulate
  QVector<GuiComponent> components = gridWidget->getComponents();
  if (components.isEmpty()) return;

  NetlistData data = generateNetlistData(components);

  QTemporaryFile tmpFile;
  tmpFile.setAutoRemove(true);
  if (!tmpFile.open()) {
    return;  // Could add a message box on failure
  }
  QTextStream out(&tmpFile);

  out << data.content;
  out.flush();

  // Parse and simulate
  NetlistParser parser;
  std::unique_ptr<Circuit> circuit =
      parser.ParseNetlist(tmpFile.fileName().toStdString());
  Simulator sim(circuit.get());
  if (simulationType == SimulationType::AC) {
    sim.SolveAC(acFrequency);
  } else {
    sim.SolveDC();
  }

  // Collect node voltages
  QHash<int, double> nodeIndexToVoltage;
  QHash<int, dcomplex> nodeIndexToVoltageComplex;
  nodeIndexToVoltage[0] = 0.0;
  nodeIndexToVoltageComplex[0] = {0.0, 0.0};
  for (Node* n : circuit->GetNodes()) {
    bool ok = false;
    int idx = QString::fromStdString(n->GetName()).toInt(&ok);
    if (ok) {
      nodeIndexToVoltage[idx] = n->voltage_;
      if (simulationType == SimulationType::AC)
        nodeIndexToVoltageComplex[idx] = n->voltage_ac_;
    }
  }

  QHash<QPoint, double> pointVoltages;
  QHash<QPoint, dcomplex> pointVoltagesComplex;
  for (auto it = data.pointToNodeIndex.constBegin();
       it != data.pointToNodeIndex.constEnd(); ++it) {
    if (nodeIndexToVoltage.contains(it.value())) {
      pointVoltages[it.key()] = nodeIndexToVoltage[it.value()];
    }
    if (nodeIndexToVoltageComplex.contains(it.value())) {
      pointVoltagesComplex[it.key()] = nodeIndexToVoltageComplex[it.value()];
    }
  }
  gridWidget->setNodeVoltages(pointVoltages);
  if (simulationType == SimulationType::AC)
    gridWidget->setComplexNodeVoltages(pointVoltagesComplex);

  // Collect per-component currents using compOrder mappings
  QHash<int, double> compCurrents;
  QHash<int, dcomplex> compCurrentsComplex;
  for (auto it = data.compOrder.constBegin(); it != data.compOrder.constEnd();
       ++it) {
    int guiIdx = it.key();
    QString t = it.value().first;
    int ord = it.value().second;
    double current = 0.0;
    dcomplex complexCurrent = {0.0, 0.0};
    if (t == "R") {
      auto& vec = circuit->GetResistors();
      if (ord - 1 >= 0 && ord - 1 < (int)vec.size() && vec[ord - 1]) {
        current = vec[ord - 1]->current_;
        if (simulationType == SimulationType::AC)
          complexCurrent = vec[ord - 1]->complex_current_;
      }
    } else if (t == "L") {
      std::vector<Component*> vec;
      if (simulationType == SimulationType::AC)
        vec = circuit->GetInductors();
      else
        vec = circuit->GetVoltageSources();
      if (ord - 1 >= 0 && ord - 1 < (int)vec.size() && vec[ord - 1]) {
        current = vec[ord - 1]->current_;
        if (simulationType == SimulationType::AC)
          complexCurrent = vec[ord - 1]->complex_current_;
      }
    } else if (t == "V") {
      auto& vec = circuit->GetVoltageSources();
      if (ord - 1 >= 0 && ord - 1 < (int)vec.size() && vec[ord - 1]) {
        current = vec[ord - 1]->current_;
        if (simulationType == SimulationType::AC)
          complexCurrent = vec[ord - 1]->complex_current_;
      }
    } else if (t == "I") {
      auto& vec = circuit->GetCurrentSources();
      if (ord - 1 >= 0 && ord - 1 < (int)vec.size() && vec[ord - 1]) {
        current = vec[ord - 1]->GetValue();
        if (simulationType == SimulationType::AC)
          complexCurrent = vec[ord - 1]->complex_current_;
      }
    } else if (t == "C") {
      auto& vec = circuit->GetCapacitors();
      if (ord - 1 >= 0 && ord - 1 < (int)vec.size() && vec[ord - 1]) {
        current = 0.0;
        if (simulationType == SimulationType::AC)
          complexCurrent = vec[ord - 1]->complex_current_;
      }
    } else if (t == "O") {
      auto& vec = circuit->GetOpAmps();
      if (ord - 1 >= 0 && ord - 1 < (int)vec.size() && vec[ord - 1]) {
        current = vec[ord - 1]->current_;
        if (simulationType == SimulationType::AC)
          complexCurrent = vec[ord - 1]->complex_current_;
      }
    } else if (t == "E") {
      auto& vec = circuit->GetVCVSs();
      if (ord - 1 >= 0 && ord - 1 < (int)vec.size() && vec[ord - 1]) {
        current = vec[ord - 1]->current_;
        if (simulationType == SimulationType::AC)
          complexCurrent = vec[ord - 1]->complex_current_;
      }
    } else if (t == "G") {
      auto& vec = circuit->GetVCCSs();
      if (ord - 1 >= 0 && ord - 1 < (int)vec.size() && vec[ord - 1]) {
        current = vec[ord - 1]->current_;
        if (simulationType == SimulationType::AC)
          complexCurrent = vec[ord - 1]->complex_current_;
      }
    } else if (t == "H") {
      auto& vec = circuit->GetCCVSs();
      if (ord - 1 >= 0 && ord - 1 < (int)vec.size() && vec[ord - 1]) {
        current = vec[ord - 1]->current_;
        if (simulationType == SimulationType::AC)
          complexCurrent = vec[ord - 1]->complex_current_;
      }
    } else if (t == "F") {
      auto& vec = circuit->GetCCCSs();
      if (ord - 1 >= 0 && ord - 1 < (int)vec.size() && vec[ord - 1]) {
        current = vec[ord - 1]->current_;
        if (simulationType == SimulationType::AC)
          complexCurrent = vec[ord - 1]->complex_current_;
      }
    }
    compCurrents[guiIdx] = current;
    compCurrentsComplex[guiIdx] = complexCurrent;
  }
  gridWidget->setComponentCurrents(compCurrents);
  if (simulationType == SimulationType::AC)
    gridWidget->setComponentComplexCurrents(compCurrentsComplex);

  // In onRunSimulation(), after collecting pointVoltages, add this:
  // Special handling for Op-Amp output terminals
  for (int idx = 0; idx < components.size(); ++idx) {
    const auto& component = components[idx];
    if (component.type == "Op-Amp") {
      QVector<QPoint> terminals = component.getTerminals();
      if (terminals.size() >= 3) {
        // terminals[0] = positive input
        // terminals[1] = negative input
        // terminals[2] = output

        // Get the output node from the circuit
        auto& opamps = circuit->GetOpAmps();
        if (idx < (int)opamps.size() && opamps[idx]) {
          Node* outNode = opamps[idx]->GetOutNode();
          if (outNode) {
            bool ok = false;
            int outIdx = QString::fromStdString(outNode->GetName()).toInt(&ok);
            if (ok && nodeIndexToVoltage.contains(outIdx)) {
              // Override the output terminal voltage
              pointVoltages[terminals[2]] = nodeIndexToVoltage[outIdx];
              if (simulationType == SimulationType::AC &&
                  nodeIndexToVoltageComplex.contains(outIdx)) {
                pointVoltagesComplex[terminals[2]] =
                    nodeIndexToVoltageComplex[outIdx];
              }
            }
          }
        }
      }
    }
  }
  gridWidget->setNodeVoltages(pointVoltages);
}

void MainWindow::onActionToggled(bool checked) {
  QAction* action = qobject_cast<QAction*>(sender());
  if (checked) {
    for (QAction* otherAction : actionGroup->actions()) {
      if (otherAction != action) {
        otherAction->setChecked(false);
      }
    }
    gridWidget->setCurrentComponentType(action->text());
    gridWidget->cancelSelection();
  } else {
    bool anyChecked = false;
    for (QAction* otherAction : actionGroup->actions()) {
      if (otherAction->isChecked()) {
        anyChecked = true;
        break;
      }
    }
    if (!anyChecked) {
      gridWidget->setCurrentComponentType("");
    }
  }
}

void MainWindow::onSaveAs() {
  // Try to find the project root relative to the build directory
  QDir dir(QCoreApplication::applicationDirPath());
  while (dir.dirName() != "circuit-simulator-1" && dir.cdUp()) {
  }

  // Save Netlist
  QDir netlistDir = dir;
  if (!netlistDir.exists("netlists")) {
    netlistDir.mkdir("netlists");
  }
  netlistDir.cd("netlists");

  QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
  QString fileName = QString("circuit-%1.net").arg(timestamp);
  QString filePath = netlistDir.absoluteFilePath(fileName);

  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QMessageBox::warning(this, "Error", "Could not save file: " + filePath);
    return;
  }

  QTextStream out(&file);
  QVector<GuiComponent> components = gridWidget->getComponents();

  NetlistData data = generateNetlistData(components);
  out << data.content;

  file.flush();
  file.close();

  // Save Falstad
  QDir falstadDir = dir;
  if (!falstadDir.exists("src/gui/falstad_")) {
    falstadDir.mkpath("src/gui/saved_circuits");
  }
  falstadDir.cd("src/gui/saved_circuits");

  QString falstadFileName = QString("circuit-%1.txt").arg(timestamp);
  QString falstadFilePath = falstadDir.absoluteFilePath(falstadFileName);
  QFile falstadFile(falstadFilePath);
  if (falstadFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream fOut(&falstadFile);
    fOut << generateFalstadContent(components);
    falstadFile.flush();
    falstadFile.close();
  }

  if (statusBar()) {
    statusBar()->showMessage(
        tr("Saved to %1 and %2").arg(fileName).arg(falstadFileName), 3000);
  }
}

void MainWindow::onOpenFile() {
  QDir dir(QCoreApplication::applicationDirPath());
  while (dir.dirName() != "circuit-simulator-1" && dir.cdUp()) {
  }
  QString initialPath = dir.absolutePath();
  if (dir.exists("src/gui/saved_circuits")) {
    initialPath = dir.absoluteFilePath("src/gui/saved_circuits");
  }

  QString fileName = QFileDialog::getOpenFileName(
      this, tr("Open Circuit File"), initialPath,
      tr("Circuit Files (*.txt *.circuit);;All Files (*)"));

  if (fileName.isEmpty()) return;

  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QMessageBox::warning(
        this, tr("Error"),
        tr("Cannot read file %1:\n%2.").arg(fileName).arg(file.errorString()));
    return;
  }

  QTextStream in(&file);
  QString content = in.readAll();
  gridWidget->loadFromFalstad(content);

  if (statusBar()) {
    statusBar()->showMessage(tr("Loaded %1").arg(fileName), 3000);
  }
}

// Write Netlist
MainWindow::NetlistData MainWindow::generateNetlistData(
    const QVector<GuiComponent>& components) {
  NetlistData data;
  if (components.isEmpty()) return data;

  std::map<QPoint, int, QPointLessThan> nodeMap;
  int nodeCounter = 0;
  for (const auto& component : components) {
    for (const auto& pt : component.getTerminals()) {
      if (nodeMap.find(pt) == nodeMap.end()) {
        nodeMap[pt] = nodeCounter++;
      }
    }
  }

  DSU dsu(nodeCounter);

  for (const auto& c : components) {
    if (c.type == "Wire") {
      int u = nodeMap[c.startPoint];
      int v = nodeMap[c.endPoint];
      dsu.unite(u, v);
    }
  }

  std::set<int> groundedRoots;
  for (const auto& c : components) {
    if (c.type == "GND") {
      int u = nodeMap[c.startPoint];
      groundedRoots.insert(dsu.find(u));
    }
  }

  std::map<int, int> finalNodeMap;
  int finalNodeCounter = groundedRoots.empty() ? 0 : 1;
  for (int root : groundedRoots) {
    finalNodeMap[root] = 0;
  }
  static std::map<QString, int> typeCounters;
  typeCounters.clear();

  QTextStream out(&data.content);

  if (simulationType == SimulationType::AC) {
    out << ".AC " << acFrequency << "\n";
  } else {
    out << ".DC\n";
  }

  for (int idx = 0; idx < components.size(); ++idx) {
    const auto& component = components[idx];
    if (component.type == "Wire" || component.type == "GND") continue;

    QString typeChar;
    if (component.type == "Resistor")
      typeChar = "R";
    else if (component.type == "Voltage Source")
      typeChar = "V";
    else if (component.type == "Inductor")
      typeChar = "L";
    else if (component.type == "Capacitor")
      typeChar = "C";
    else if (component.type == "Current Source")
      typeChar = "I";
    else if (component.type == "Op-Amp")
      typeChar = "O";
    else if (component.type == "VCVS")
      typeChar = "E";
    else if (component.type == "VCCS")
      typeChar = "G";
    else if (component.type == "CCVS")
      typeChar = "H";
    else if (component.type == "CCCS")
      typeChar = "F";

    QVector<QPoint> terminals = component.getTerminals();
    QVector<int> nodeIndices;
    for (const auto& pt : terminals) {
      int root = dsu.find(nodeMap[pt]);
      if (finalNodeMap.find(root) == finalNodeMap.end())
        finalNodeMap[root] = finalNodeCounter++;
      nodeIndices.append(finalNodeMap[root]);
    }

    QString valueStr;
    if (component.type == "Resistor" &&
        component.parameters.contains("Resistance")) {
      valueStr = " " + component.parameters["Resistance"].toString();
    } else if (component.type == "Voltage Source" &&
               component.parameters.contains("Voltage")) {
      valueStr = " " + component.parameters["Voltage"].toString();
    } else if (component.type == "Inductor" &&
               component.parameters.contains("Inductance")) {
      valueStr = " " + component.parameters["Inductance"].toString();
    } else if (component.type == "Capacitor" &&
               component.parameters.contains("Capacitance")) {
      valueStr = " " + component.parameters["Capacitance"].toString();
    } else if (component.type == "Current Source" &&
               component.parameters.contains("Current")) {
      valueStr = " " + component.parameters["Current"].toString();
    } else if (component.type == "VCVS" &&
               component.parameters.contains("Gain")) {
      valueStr = " " + component.parameters["Gain"].toString();
    } else if (component.type == "VCCS" &&
               component.parameters.contains("Transconductance")) {
      valueStr = " " + component.parameters["Transconductance"].toString();
    } else if (component.type == "CCVS" &&
               component.parameters.contains("Transresistance")) {
      valueStr = " " + component.parameters["Transresistance"].toString();
    } else if (component.type == "CCCS" &&
               component.parameters.contains("Gain")) {
      valueStr = " " + component.parameters["Gain"].toString();
    } else if (component.type == "Op-Amp" &&
               component.parameters.contains("Gain")) {
      valueStr = " " + component.parameters["Gain"].toString();
    }

    QString key = typeChar.isEmpty() ? "X" : typeChar;
    int ordinal = ++typeCounters[key];
    data.compOrder[idx] = qMakePair(typeChar, ordinal);
    QString compId = typeChar + QString::number(ordinal);

    if (component.type == "Op-Amp") {
      // Oname pos neg out gain
      // terminals: 0=pos, 1=neg, 2=out
      if (nodeIndices.size() >= 3) {
        out << compId << " " << nodeIndices[0] << " " << nodeIndices[1] << " "
            << nodeIndices[2] << valueStr << "\n";
      }
    } else if (component.type == "VCVS" || component.type == "VCCS" ||
               component.type == "CCVS" || component.type == "CCCS") {
      // Ename n+ n- nc+ nc- val
      // terminals: 0=out+, 1=out-, 2=ctrl+, 3=ctrl-
      if (nodeIndices.size() >= 4) {
        out << compId << " " << nodeIndices[0] << " " << nodeIndices[1] << " "
            << nodeIndices[2] << " " << nodeIndices[3] << valueStr << "\n";
      }
    } else {
      if (nodeIndices.size() >= 2) {
        out << compId << " " << nodeIndices[0] << " " << nodeIndices[1]
            << valueStr << "\n";
      }
    }
  }
  out.flush();

  // Recreate point->node index mapping
  for (auto it = nodeMap.begin(); it != nodeMap.end(); ++it) {
    int root = dsu.find(it->second);
    int finalIdx = finalNodeMap[root];
    data.pointToNodeIndex[it->first] = finalIdx;
  }

  return data;
}

QString MainWindow::generateFalstadContent(
    const QVector<GuiComponent>& components) {
  QString content;
  QTextStream out(&content);

  out << "$ 1 0.000005 10.20027730826997 50 5 50 5e-11\n";

  for (const auto& comp : components) {
    int x1 = comp.startPoint.x();
    int y1 = comp.startPoint.y();
    int x2 = comp.endPoint.x();
    int y2 = comp.endPoint.y();

    if (comp.type == "Resistor") {
      double val = comp.parameters.value("Resistance", 1000.0).toDouble();
      out << "r " << x1 << " " << y1 << " " << x2 << " " << y2 << " 0 " << val
          << "\n";
    } else if (comp.type == "Wire") {
      out << "w " << x1 << " " << y1 << " " << x2 << " " << y2 << " 0\n";
    } else if (comp.type == "Voltage Source") {
      double val = comp.parameters.value("Voltage", 5.0).toDouble();
      // v x1 y1 x2 y2 flags frequency phase_offset voltage rms
      out << "v " << x1 << " " << y1 << " " << x2 << " " << y2 << " 0 0 40 "
          << val << " 0 0 0.5\n";
    } else if (comp.type == "Capacitor") {
      double val = comp.parameters.value("Capacitance", 1e-6).toDouble();
      out << "c " << x1 << " " << y1 << " " << x2 << " " << y2 << " 0 " << val
          << " 0.001\n";
    } else if (comp.type == "Inductor") {
      double val = comp.parameters.value("Inductance", 1e-3).toDouble();
      out << "l " << x1 << " " << y1 << " " << x2 << " " << y2 << " 0 " << val
          << " 0.001\n";
    } else if (comp.type == "Current Source") {
      double val = comp.parameters.value("Current", 1.0).toDouble();
      out << "i " << x1 << " " << y1 << " " << x2 << " " << y2 << " 0 " << val
          << "\n";
    } else if (comp.type == "GND") {
      // Falstad uses 'g' for ground
      out << "g " << x1 << " " << y1 << " " << x2 << " " << y2 << " 0\n";
    } else if (comp.type == "Op-Amp") {
      double val = comp.parameters.value("Gain", 1e6).toDouble();
      // a x1 y1 x2 y2 flags max_out min_out gain rail+ rail-
      // Calculate midpoint between inputs (positive and negative terminals)
      int midX = (x1 + x2) / 2;
      int midY = (y1 + y2) / 2;
      // Calculate axis vector (perpendicular to input vector, pointing to
      // output)
      int dx = x2 - x1;
      int dy = y2 - y1;
      // Rotate 90 degrees: (dx, dy) -> (-dy, dx)
      int ax = -dy;
      int ay = dx;
      // Normalize to grid size
      double len = std::sqrt(ax * ax + ay * ay);
      if (len > 1e-5) {
        ax = static_cast<int>(ax / len * GRID_SIZE * 2);
        ay = static_cast<int>(ay / len * GRID_SIZE * 2);
      }
      out << "a " << midX << " " << midY << " " << (midX + 80) << " " << (midY)
          << " 8 15 -15 " << val << " 15 -15\n";
    } else if (comp.type == "VCVS") {
      // Voltage Controlled Voltage Source
      // Format: E x1 y1 x2 y2 flags gain
      // For dependent sources, we need all 4 terminals
      QVector<QPoint> terminals = comp.getTerminals();
      if (terminals.size() >= 4) {
        double gain = comp.parameters.value("Gain", 1.0).toDouble();
        // Output terminals
        int ox1 = terminals[0].x();
        int oy1 = terminals[0].y();
        int ox2 = terminals[1].x();
        int oy2 = terminals[1].y();
        // Control terminals
        int cx1 = terminals[2].x();
        int cy1 = terminals[2].y();
        int cx2 = terminals[3].x();
        int cy2 = terminals[3].y();

        out << "212 " << ox1 << " " << oy1 << " " << cx1 << " " << cy1
            << " 0 2 " << gain << "*(a-b)\n";
      }
    } else if (comp.type == "VCCS") {
      // Voltage Controlled Current Source
      QVector<QPoint> terminals = comp.getTerminals();
      if (terminals.size() >= 4) {
        double gm = comp.parameters.value("Transconductance", 1.0).toDouble();
        int ox1 = terminals[0].x();
        int oy1 = terminals[0].y();
        int ox2 = terminals[1].x();
        int oy2 = terminals[1].y();
        // Control terminals
        int cx1 = terminals[2].x();
        int cy1 = terminals[2].y();
        int cx2 = terminals[3].x();
        int cy2 = terminals[3].y();

        out << "213 " << ox1 << " " << oy1 << " " << cx1 << " " << cy1
            << " 0 2 " << gm << "*(a-b)\n";
      }
    } else if (comp.type == "CCVS") {
      // Current Controlled Voltage Source
      QVector<QPoint> terminals = comp.getTerminals();
      if (terminals.size() >= 4) {
        double rm = comp.parameters.value("Transresistance", 1.0).toDouble();
        int ox1 = terminals[0].x();
        int oy1 = terminals[0].y();
        int ox2 = terminals[1].x();
        int oy2 = terminals[1].y();
        // Control terminals
        int cx1 = terminals[2].x();
        int cy1 = terminals[2].y();
        int cx2 = terminals[3].x();
        int cy2 = terminals[3].y();

        out << "214 " << ox1 << " " << oy1 << " " << cx1 << " " << cy1
            << " 0 2 " << rm << "*(a-b)\n";
      }
    } else if (comp.type == "CCCS") {
      // Current Controlled Current Source
      QVector<QPoint> terminals = comp.getTerminals();
      if (terminals.size() >= 4) {
        double gain = comp.parameters.value("Gain", 1.0).toDouble();
        int ox1 = terminals[0].x();
        int oy1 = terminals[0].y();
        int ox2 = terminals[1].x();
        int oy2 = terminals[1].y();
        // Control terminals
        int cx1 = terminals[2].x();
        int cy1 = terminals[2].y();
        int cx2 = terminals[3].x();
        int cy2 = terminals[3].y();

        out << "215 " << ox1 << " " << oy1 << " " << cx1 << " " << cy1
            << " 0 2 " << gain << "*(a-b)\n";
      }
    }
  }

  return content;
}
