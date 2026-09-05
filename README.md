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

To install the required Qt libraries, run:

```bash
sudo apt update
sudo apt install qtbase5-dev
```

Install cmake
```
sudo apt install cmake
```

# Building and usage
This project uses the CMake build tool. To build and run the software, follow these steps:
1. Create a build subdirectory in the project root
```
mkdir build
cd build
```
2. Create the CMake files
```
cmake ..
```
3. Build the software
```
make
```
To run the GUI circuit simulator, execute
```
./gui/circuit_sim_gui
```

After building the software, there are three different executables available:

| Executable | Location | Description |
| :--- | :--- | :--- |
| circuit_sim | build/ | Simulates pre-built circuits and prints results to the standard output stream. |
| circuit_sim_gui | build/gui/ | The graphical user interface for building and displaying circuits. |
| sim_tests | build/tests/ | Executable containing unit tests for all critical parts of the software. |
