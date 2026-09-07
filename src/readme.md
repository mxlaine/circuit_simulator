# Source guide

The core library is shared by the Qt application, command-line examples, and tests.

| Location | Responsibility |
| --- | --- |
| [core/circuit.cpp](core/circuit.cpp) and [core/circuit.hpp](core/circuit.hpp) | Component storage, node discovery, and circuit construction |
| [core/](core/) | Nodes and component types, including dependent sources and ideal op-amps |
| [sim/simulator.cpp](sim/simulator.cpp) | DC/AC matrix assembly, solving, and current calculation |
| [io/netlist_parser.cpp](io/netlist_parser.cpp) | Parse the project's SPICE-like text format into a circuit |
| [main.cpp](main.cpp) | Hard-coded command-line examples |
| [gui/mainwindow.cpp](gui/mainwindow.cpp) | File operations and simulation controls |
| [gui/gridwidget.cpp](gui/gridwidget.cpp) | Circuit editing, Falstad import/export, and result tooltips |
| [gui/plotdialog.cpp](gui/plotdialog.cpp) | Reconstruct sinusoidal waveforms from AC results using QCustomPlot |

## Solver

Start with `Simulator::FillAMatrix` for DC stamping and `Simulator::SolveAC`
for the complex-valued equivalent. The unknowns are non-ground node voltages
and additional branch currents needed for voltage sources and constraints.
Resistors contribute conductance; in AC, capacitors and inductors contribute
`jωC` and `1/(jωL)`.

For resistors and independent voltage sources, the MNA matrix has the familiar
`[G B; Bᵀ 0]` structure. Dependent sources and ideal op-amps add stamps that do
not generally preserve that symmetry. Both solvers use dense Eigen matrices
and column-pivoted Householder QR.

The parser converts DC inductors to short circuits before solving. This
conversion is not performed by the direct circuit-construction API.
`CheckCircuitTopology` is a preliminary check, not a complete validation of
the assembled system; see the [limitations](../README.md#file-formats-and-limitations).

## Following an example

[The Butterworth demo](../examples/butterworth.cpp) constructs a circuit,
sweeps the AC solver, and checks output magnitude against the analytical
response. [The tests](../tests/) contain smaller examples for individual
component types and parser errors.
