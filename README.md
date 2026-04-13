# QVis — Quantum Circuit Simulator

An interactive educational tool for building quantum circuits and visualizing state evolution, probabilities, and measurement outcomes.

Built for BU ENG EC 327 (Spring 2026, Prof. Douglas Densmore).

## Architecture

```
┌────────────┐   pybind11   ┌──────────────┐   HTTP/JSON   ┌──────────────────┐
│  C++ Engine │ ──────────► │  FastAPI      │ ◄──────────► │  Vite + D3.js    │
│  (Model)    │             │  (Controller) │              │  (View)          │
└────────────┘             └──────────────┘              └──────────────────┘
```

- **C++ Engine** — quantum state vectors, unitary gate matrices, Kronecker expansion, measurement sampling (little-endian qubit ordering)
- **pybind11 Bindings** — expose the engine to Python via scikit-build-core
- **FastAPI Backend** — circuit execution, step-by-step state snapshots, von Neumann entropy
- **Vite + D3.js Frontend** — circuit picker, animated state evolution bars with phase coloring, measurement histograms

## Building and Running

### Prerequisites

- C++17 compiler, CMake 3.14+
- Python 3.10+
- Node.js 18+

### 1. Install the engine + backend

```bash
pip install -e ".[dev]"
```

This uses scikit-build-core to compile the C++ engine with pybind11 bindings and installs the FastAPI backend with all dependencies.

### 2. Install frontend dependencies

```bash
cd frontend && npm install
```

### 3. Start the servers

```bash
# Terminal 1 — backend (port 8000)
python -m backend.app

# Terminal 2 — frontend (port 3000)
cd frontend && npm run dev
```

Open `http://localhost:3000` in your browser.

### Running tests

```bash
# C++ unit tests (GoogleTest)
cmake -S engine -B engine/build && cmake --build engine/build && ctest --test-dir engine/build

# Python tests (pytest)
pytest

# Frontend tests (vitest)
cd frontend && npm test
```

## Demo Circuits

| Circuit | What it does |
|---------|-------------|
| **Coin Flip (Hadamard)** | A single Hadamard gate puts one qubit into 50/50 superposition. Measurement produces roughly equal counts of \|0⟩ and \|1⟩. |
| **Bell State (Entanglement)** | Hadamard on qubit 0, then CNOT linking qubits 0 and 1. Creates maximal entanglement — only \|00⟩ and \|11⟩ appear, perfectly correlated. |

## Screenshot

<!-- Replace with an actual screenshot of the app -->
![QVis screenshot](docs/screenshot-placeholder.png)

## Team GAM

- **Johnny Garcia** — johnnyg0@bu.edu
- **Fairuz Abushgarah** — f4416@bu.edu
- **Micah Mahone** — mmahone@bu.edu
- Mentor: **Pravi Samaratunga** — pravi@bu.edu
