# ADR-001: QVis Architecture

**Status:** Accepted  
**Date:** 2026-04-12  
**Authors:** Team GAM (Fairuz Abushgarah, Maggie Mahone, Johnny Garcia)

## Context

QVis is an educational quantum circuit simulator built for BU ENG-EC-327. Users drag-and-drop gates onto qubit wires, run the circuit, and observe output probabilities. The system follows a strict Model-View-Controller split: C++ owns the math, Python owns routing and orchestration, and JS owns rendering. The target is a working demo, not a production service.

## Decisions

### 1. Engine — C++17, StateVector class, gate functions

The simulation engine is a C++17 static library. The central data structure is `StateVector`, a wrapper around `std::vector<std::complex<double>>` using little-endian qubit ordering (qubit 0 = least significant bit). Gates are free functions that take a `StateVector&` and qubit indices, build the full unitary via Kronecker expansion, and apply it in place. All gate code generalizes to n qubits — no hard-coded 2-qubit limits. Unit tests use GoogleTest, fetched via CMake `FetchContent`.

### 2. Bindings — pybind11, single extension module

A single pybind11 module (`qvis_engine_ext`) exposes all engine classes and functions to Python. The module is compiled by CMake when `BUILD_PYTHON_BINDINGS=ON` and installed into the `bindings/` package via scikit-build-core. Python code never performs simulation math directly — it calls into the extension module.

### 3. Backend — FastAPI, POST /api/sample

The backend is a FastAPI application. The primary endpoint is:

    POST /api/sample

**Request:**
```json
{
  "circuit": [
    {"gate": "h", "targets": [0]},
    {"gate": "cx", "targets": [0, 1]}
  ],
  "shots": 1000
}
```

**Response:**
```json
{
  "counts": {"00": 503, "11": 497}
}
```

The backend deserializes the circuit JSON, builds the corresponding gate sequence via the pybind11 bindings, executes the circuit, samples the final state vector `shots` times, and returns bitstring counts. A `/health` endpoint exists for CI and readiness checks.

### 4. Frontend — Vite + D3.js, plain HTML/JS

The frontend uses Vite as the dev server and bundler. D3.js renders probability histograms; Three.js renders 3D state visualizations (Bloch sphere, probability clouds). The circuit builder is plain HTML/CSS with drag-and-drop — no React or framework. The frontend communicates with the backend via `fetch()`.

### 5. Transport — JSON over HTTP, dev proxy

All communication between frontend and backend is JSON over HTTP. In development, the Vite dev server runs on port 3000 and proxies `/api/*` requests to the FastAPI backend on port 8000. This avoids CORS issues during development and mirrors how a production reverse proxy would work.

## Circuit JSON Schema

A circuit is a JSON array of gate operations, executed in order:

| Field     | Type       | Description                                                      |
|-----------|------------|------------------------------------------------------------------|
| `gate`    | string     | Gate name: `"h"`, `"x"`, `"y"`, `"z"`, `"cx"`, `"cz"`, etc.    |
| `targets` | int array  | Qubit indices. Single-qubit gates: `[q]`. Two-qubit: `[control, target]`. |

**Example — Bell state (|00⟩ + |11⟩) / √2:**

```json
[
  {"gate": "h",  "targets": [0]},
  {"gate": "cx", "targets": [0, 1]}
]
```

This applies a Hadamard to qubit 0, then a CNOT with qubit 0 as control and qubit 1 as target. Sampling this circuit should produce roughly 50/50 counts of `"00"` and `"11"`.

## Consequences

- **No WebSocket / streaming** — the request-response model is simple but means the frontend polls or waits for results. Acceptable at demo scale.
- **No React** — keeps the frontend dependency footprint small and avoids a build-tooling learning curve, but means manual DOM management for the circuit builder.
- **pybind11 build complexity** — requires CMake + a C++ compiler in CI and on every developer machine. Mitigated by scikit-build-core handling the build automatically during `pip install`.
- **Single-process backend** — no task queue or background workers. Long-running simulations (many qubits) will block the request. Acceptable for the demo's scope (≤ ~10 qubits).
