# Core (`src/core/`)

## Circuit Representation
- **node.hpp**: Electrical node representation. Holds DC voltages and AC phasor values.
- **component.hpp**: Abstract base class for all components. Defines terminal nodes, values, and current tracking.
- **circuit.hpp / circuit.cpp**: Container for the entire circuit. Manages nodes, components, and ground.

## Passive Components
- **resistor.hpp**: Resistor (conductance for MNA).
- **capacitor.hpp**: Capacitor (AC impedance handling).
- **inductor.hpp**: Inductor (AC impedance handling).

## Sources
- **voltage_source.hpp**: Independent voltage source.
- **current_source.hpp**: Independent current source.
- **short_circuit.hpp**: Zero-voltage source for current sensing.

## Dependent Sources
- **vcvs.hpp**: Voltage-controlled voltage source.
- **vccs.hpp**: Voltage-controlled current source.
- **ccvs.hpp**: Current-controlled voltage source.
- **cccs.hpp**: Current-controlled current source.

## Active Components
- **opamp.hpp**: Ideal operational amplifier (V+ = V− constraint).

## Simulator
**simulator.hpp / simulator.cpp**
- Implements Modified Nodal Analysis
- DC operating-point analysis
- AC phasor-domain analysis
- Matrix assembly and topology validation

## Netlist Parsing
- **netlist_parser.hpp / netlist_parser.cpp**: SPICE-like netlist parser. Produces `Circuit` objects and simulation directives.


# GUI (`src/gui/`)

## Application
- **main.cpp**: Qt entry point.
- **mainwindow.hpp / mainwindow.cpp**: Main window, file operations, simulation controls.

## Circuit Editor
- **gridwidget.hpp / gridwidget.cpp**
  - Interactive canvas
  - Component placement with grid snapping
  - Selection, movement, panning, zooming
  - Rendering for all components
  - Tooltips for simulation results
  - Import/export of Falstad circuits

- **component.hpp** (GUI): GUI-side component metadata (type, position, parameters).

## Dialogs
- **componentdialog.hpp / componentdialog.cpp**: Component parameter editing.
- **plotdialog.hpp / plotdialog.cpp**: Waveform visualization using QCustomPlot.


# Simulation Flow
NetlistParser → Circuit → Simulator → Results  
GUI (GridWidget/MainWindow) → Netlist → Simulator → Tooltip/Plot Output


# Supported Features
- DC operating-point analysis
- AC frequency-domain analysis (complex phasors)
- Full component set: R, C, L, independent sources, dependent sources, op-amps
- Qt GUI circuit editor with real-time result display
- Netlist-based file I/O and Falstad import
- AC waveform plotting
- MNA matrix formulation


# Short MNA Explanation
The simulator constructs and solves:

[ G   B ] [ v ] = [ i ]  
[ B^T 0 ] [ j ]   [ e ]

Where:
- **G**: Conductance/admittance matrix
- **B**: Voltage-source incidence matrix
- **v**: Node voltages
- **j**: Source branch currents
- **i**: Current injections
- **e**: Voltage source values

AC analysis uses complex admittance values for R, L, and C.
