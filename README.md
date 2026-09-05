# Circuit Simulator 1

A simple circuit simulator project. Simulates basic electronic circuits and components, allowing users to experiment and visualize circuit behavior.

# Features

- Components
    - Resistor
    - Capacitor
    - Inductor
    - AC/DC voltage source
    - AC/DC current source
    - Dependent sources
- GUI
    - Build and display circuits
    - Save and load from files
    - Time domain graphing


# Installation instructions

Clone this repo:

```bash
git clone https://github.com/mxlaine/circuit_simulator.git
cd circuit_simulator
```

The following commands are for Ubuntu/Debian Linux. You need a C++17 compiler,
CMake 3.16 or newer, and Qt development libraries. Eigen, Catch2, and QCustomPlot
are included in `libs/`.

Install the compiler, build tools, Git, and Qt 5:

```bash
sudo apt update
sudo apt install build-essential cmake git qtbase5-dev
```


# Building and usage

Run these commands from the project root, where `CMakeLists.txt` is located.
CMake creates the build directory automatically; the commands also work when
rebuilding an existing checkout.

```bash
cmake -S . -B build
cmake --build build --parallel 2
```

Launch the GUI from the project root:

```bash
./build/gui/circuit_sim_gui
```

The GUI requires a graphical desktop session (or WSL with working GUI support).

Run the command-line examples and the test suite from the project root:

```bash
./build/circuit_sim
cd build && ctest --output-on-failure
```

For a command-line-only build, Qt is not required. Use a separate build directory (again, running from project root):

```bash
cmake -S . -B build-cli -DBUILD_GUI=OFF
cmake --build build-cli --parallel 2
./build-cli/circuit_sim
cd build-cli && ctest --output-on-failure
```

After building the software, there are three different executables available:

| Executable | Location | Description |
| :--- | :--- | :--- |
| circuit_sim | build/ | Simulates pre-built circuits and prints results to the standard output stream. |
| circuit_sim_gui | build/gui/ | The graphical user interface for building and displaying circuits. |
| sim_tests | build/tests/ | Unit tests, also run by CTest. |
