# QVis — Quantum Circuit Simulator

Team GAM's final project for BU ENG-EC-327 (Spring 2026, Prof. Douglas Densmore).

## What this is

An interactive software application that lets users drag-and-drop quantum gates onto visual qubit wires, run the circuit, and observe the resulting probability distribution, 3D state visualizations, and a von Neumann entropy measure of system complexity. The goal is a clear, intuitive educational tool for learning quantum computing concepts.

## Architecture (Model-View-Controller)

- **Model** — C++ simulation engine under `engine/`. Quantum states live as complex-valued vectors, gates as unitary matrices, circuit execution via Kronecker products and matrix–vector multiplication. This is the performance-critical layer and owns all the math.
- **View** — JS/HTML/CSS frontend under `frontend/`. Three.js for 3D state visualizations (Bloch sphere, probability clouds), D3.js for probability charts, plain HTML/CSS for the drag-and-drop circuit builder.
- **Controller** — Python backend under `backend/` (Flask or FastAPI, to be decided). Routes user input, manages circuit state, computes entropy, and calls into the C++ engine via pybind11 bindings under `bindings/`.

## Directory layout

```
QVis/
├── engine/          # C++ simulation engine (Model)
│   ├── src/         # gate implementations, state vector, circuit runner
│   ├── include/     # public headers
│   ├── tests/       # C++ unit tests
│   └── Makefile
├── bindings/        # pybind11 wrapper exposing engine to Python
├── backend/         # Python (Flask/FastAPI) + entropy + session mgmt
│   ├── app.py
│   ├── requirements.txt
│   └── tests/       # pytest
├── frontend/        # JS/HTML/CSS + Three.js + D3.js
│   ├── src/
│   ├── index.html
│   └── package.json
├── docs/            # proposal, design notes, meeting minutes
├── CLAUDE.md        # (this file)
└── README.md
```

## Build

```
make -C engine                  # builds the C++ engine library
pip install -e .                # installs Python package with pybind11 bindings
cd frontend && npm install      # installs JS deps
```

## Run

```
cd backend && python app.py     # starts Flask/FastAPI server
cd frontend && npm run dev      # starts frontend dev server
```

## Test

```
make -C engine test             # C++ unit tests
pytest backend/tests            # Python tests
cd frontend && npm test         # JS tests (once added)
```

## Conventions

- **Qubit ordering** — little-endian. Qubit 0 is the least significant bit. State vector index `i` corresponds to the binary representation of `i` where bit 0 = qubit 0. Document this anywhere a gate matrix is written down.
- **Gate matrix convention** — row index = output basis state, column index = input basis state. Apply gates as `|ψ'⟩ = U |ψ⟩`.
- **Complex numbers** — `std::complex<double>` throughout the engine. No mixing precisions.
- **Tests before wiring** — any new gate needs a C++ unit test (matrix correctness + action on a known state) before it gets exposed through pybind11 or shown in the frontend.
- **n-qubit generality** — all gate code must generalize to n qubits via Kronecker expansion. No hard-coded 2-qubit limits.
- **MVC boundaries are firm** — simulation math lives in C++. Python handles routing and entropy. JS handles rendering. Don't blur these.

## What NOT to do

- Don't add new runtime dependencies without flagging to the team first. Keep the build simple for the demo.
- Don't put simulation math in the Python or JS layer.
- Don't push directly to `main`. Feature branches + PRs only.
- Don't commit secrets, API keys, local build artifacts (`engine/build/`, `node_modules/`, `__pycache__/`), or editor config files. `.gitignore` handles most of this.
- Don't let Claude Code run destructive git operations (`reset --hard`, `push --force`, `clean -fd`) without explicit confirmation.

## Team GAM

- Fairuz Abushgarah — f4416@bu.edu
- Maggie (Micah) Mahone — mmahone@bu.edu
- Johnny Garcia — johnnyg0@bu.edu
- Mentor: Pravi Samaratunga — pravi@bu.edu

Weekly check-in: Sundays ~6:30 PM. Coordination on Slack (#gam workspace). Repo: github.com/BU-EC327-Spring2026/QVis.

## Working with Claude Code + Codex

This repo is set up to use the OpenAI Codex plugin inside Claude Code. Codex acts as a second-opinion reviewer and background task runner.

- `/codex:review` — quick review of current work
- `/codex:review --base main` — diff review against main
- `/codex:adversarial-review` — pushes back on design decisions (use this around the C++/pybind11 boundary)
- `/codex:rescue <task>` — delegate long-running investigation in the background
- `/codex:status`, `/codex:result` — monitor background jobs

Good moments to invoke Codex:
- After adding a new quantum gate — review for matrix convention bugs
- Before merging the pybind11 layer — adversarial review for memory ownership
- When a C++ unit test is silently wrong — `/codex:rescue` to investigate

## Session start routine

When starting a new Claude Code session in this repo:
1. Read this `CLAUDE.md`.
2. Check recent activity: `git log --oneline -20` and `git status`.
3. Look at any open PRs or in-progress branches before proposing changes.
4. For non-trivial changes, use Plan mode (Shift+Tab) before editing.
