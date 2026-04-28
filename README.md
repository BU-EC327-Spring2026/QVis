# QVis — Quantum Protocol Explorer

**EC327 Spring 2026 — Team GAM**  
A step-through quantum circuit visualizer with a C++ simulation engine, Python REST backend, and JavaScript frontend.

---

## What it does

QVis lets you step through real quantum algorithms one gate at a time, watching the statevector, measurement probabilities, and entanglement entropy evolve at each step.

Five protocols are included:

| Protocol | Qubits | Steps | What it demonstrates |
|---|---|---|---|
| Bell State | 2 | 3 | Superposition → entanglement |
| Quantum Teleportation | 3 | 6 | Entanglement as a resource |
| Grover's Search | 2 | 4 | O(√N) quantum speedup |
| Grover's (3-Qubit) | 3 | 6 | Amplitude amplification at scale |
| Deutsch-Jozsa | 2 | 4 | First proven quantum speedup |

---

## Architecture

```
qvis.html          ← single-file frontend (HTML + JS + D3)
    │
    │  GET /api/protocol/<name>
    ▼
backend.py         ← Flask REST API (port 8080)
    │
    │  subprocess("./qvis_engine bell")
    ▼
qvis_engine        ← C++ CLI binary → prints Protocol as JSON to stdout
    │
    │  #include
    ▼
quantum_engine.cpp ← statevector math, gate application, entropy, JSON serialization
```

The frontend never knows whether C++ or Python ran the simulation — the contract is JSON over HTTP. If `qvis_engine` isn't compiled, the backend automatically falls back to a pure-Python simulation that mirrors the same math.

---

## Quick start

### 1. Clone

```bash
git clone https://github.com/BU-EC327-Spring2026/QVis.git
cd QVis
```

### 2. Compile the C++ engine (recommended)

**macOS / Linux** — requires `g++` with C++17:

```bash
g++ -std=c++17 -O2 -o qvis_engine qvis_engine.cpp quantum_engine.cpp
```

> Don't have g++? On macOS: `xcode-select --install`. On Ubuntu: `sudo apt install g++`.  
> The app still works without this step — the Python fallback runs automatically.

### 3. Install Python dependencies

```bash
pip install flask flask-cors numpy
```

> Requires Python 3.8+. Check with `python3 --version`.

### 4. Start the backend

```bash
python backend.py
```

You should see:
```
QVis backend starting on http://localhost:8080
Protocols available: ['bell', 'teleport', 'grover', 'grover3', 'deutsch']
Engine: C++ (qvis_engine)        ← or "Python (fallback)" if not compiled
```

### 5. Open the frontend

Open `qvis.html` in your browser. The engine badge in the top-right corner confirms which simulation layer is running.

> **Port 8080** is used deliberately — macOS AirPlay Receiver occupies port 5000.

---

## Project structure

```
QVis/
├── qvis.html            # Frontend: circuit display, probability bars, step controls
├── backend.py           # Flask REST API: bridges frontend ↔ C++ engine
├── qvis_engine.cpp      # C++ CLI entry point: parses args, prints Protocol JSON
├── quantum_engine.cpp   # Simulation math: gates, statevectors, entropy, protocols
├── quantum_engine.h     # Header: all types, function declarations
├── main.cpp             # Dev utility: prints all protocols to stdout
└── .gitignore
```

---

## API reference

| Endpoint | Returns |
|---|---|
| `GET /api/protocols` | List of all protocol metadata |
| `GET /api/protocol/<name>` | Full protocol JSON (all steps pre-computed) |
| `GET /api/protocol/<name>/step/<n>` | Single step JSON |
| `GET /api/health` | Server status + active engine (`cpp` or `python`) |

Protocol names: `bell`, `teleport`, `grover`, `grover3`, `deutsch`

---

## How the C++ engine works

- **Statevector**: 2ⁿ complex amplitudes stored as `std::vector<std::complex<double>>`
- **Gate application**: sparse matrix-vector products via `apply_single` and `apply_controlled`
- **Polymorphic gate model**: `QuantumGate` base class with `SingleQubitGate`, `ControlledGate`, `PhaseOracleGate`, `DiffusionGate` derived classes — adding a new gate touches zero existing code
- **Entanglement entropy**: Von Neumann entropy via partial trace of the reduced density matrix
- **JSON serialization**: hand-written in `protocol_to_json` / `step_to_json` — no external dependencies

---

## Running the C++ dev binary

`main.cpp` is a standalone test utility that prints all protocols to stdout:

```bash
g++ -std=c++17 -O2 -o qvis_test main.cpp quantum_engine.cpp
./qvis_test
```

---

## Dependencies

| Layer | Dependency | Version |
|---|---|---|
| C++ engine | g++ | C++17 or later |
| Backend | Python | 3.8+ |
| Backend | Flask | any recent |
| Backend | flask-cors | any recent |
| Backend | numpy | any recent |
| Frontend | D3.js | 7.8.5 (CDN) |
| Frontend | Google Fonts | Space Mono, DM Sans (CDN) |

No package.json. No build step for the frontend.

---

## Troubleshooting

**`Engine: JS` badge in the header**  
The frontend isn't reaching the backend. Make sure `python backend.py` is running and that nothing else is on port 8080.

**`Engine: Python (fallback)` instead of C++**  
The `qvis_engine` binary isn't in the same directory as `backend.py`. Run the compile step in Quick Start §2.

**Port conflict on 8080**  
Edit the `PORT = 8080` line near the bottom of `backend.py`.

**CORS error in the browser console**  
The frontend must be opened as a local file (`file://`) or served from the same machine as the backend. `flask-cors` handles this automatically when installed.

---

*EC327 Spring 2026 — Boston University*
