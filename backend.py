"""
QVis Backend — Flask REST API
EC327 Spring 2026 — Team GAM

Endpoints:
  GET  /api/protocols            → list of available protocol names
  GET  /api/protocol/<name>      → full Protocol JSON (all steps pre-computed)
  GET  /api/protocol/<name>/step/<n>  → single Step JSON
  GET  /api/health               → server status + which engine is running

Engine priority:
  1. C++ binary (qvis_engine) called via subprocess — fastest, most accurate
  2. Pure-Python fallback — used automatically if the binary isn't compiled yet

Usage:
  pip install flask flask-cors numpy
  python backend.py
"""

from flask import Flask, jsonify, abort
from flask_cors import CORS
import os, json, subprocess, numpy as np
from functools import lru_cache

# look for the compiled C++ binary next to this script.
# os.path.abspath(__file__) gives the full path to backend.py itself.
_HERE = os.path.dirname(os.path.abspath(__file__))
_ENGINE_BIN = os.path.join(_HERE, "qvis_engine")
USE_CPP = os.path.exists(_ENGINE_BIN)  # True once you compile qvis_engine.cpp

app = Flask(__name__)
CORS(app)  # allow requests from the JS frontend on any port

# ──────────────────────────────────────────────────────────────────
#  Pure-Python simulation (used when C++ .so is not yet compiled)
#  Mirrors the logic in quantum_engine.cpp exactly.
# ──────────────────────────────────────────────────────────────────

INV_SQRT2 = 1.0 / np.sqrt(2)

GATE_H    = np.array([[INV_SQRT2,  INV_SQRT2],
                       [INV_SQRT2, -INV_SQRT2]], dtype=complex)
GATE_X    = np.array([[0, 1],[1, 0]], dtype=complex)
GATE_Z    = np.array([[1, 0],[0,-1]], dtype=complex)
GATE_I    = np.eye(2, dtype=complex)

def _kron_gate(gate_2x2, target, n_qubits):
    """Build full 2^n × 2^n matrix by tensoring with identity on other qubits."""
    ops = [GATE_I] * n_qubits
    ops[target] = gate_2x2
    result = ops[0]
    for op in ops[1:]:
        result = np.kron(result, op)
    return result

def _controlled_unitary(gate_2x2, control, target, n_qubits):
    """Build controlled-U as full matrix."""
    dim = 2 ** n_qubits
    U = np.eye(dim, dtype=complex)
    for i in range(dim):
        bits = format(i, f'0{n_qubits}b')
        if bits[control] == '1':
            # find partner with target bit flipped
            bits_list = list(bits)
            bits_list[target] = '1' if bits_list[target] == '0' else '0'
            j = int(''.join(bits_list), 2)
            # mark these indices for the gate
    # Rebuild using projection operators
    P0 = np.array([[1,0],[0,0]], dtype=complex)
    P1 = np.array([[0,0],[0,1]], dtype=complex)
    # U_full = |0><0|_c ⊗ I_t ⊗ ... + |1><1|_c ⊗ gate_t ⊗ ...
    ops_0 = [GATE_I] * n_qubits; ops_0[control] = P0
    ops_1 = [GATE_I] * n_qubits; ops_1[control] = P1; ops_1[target] = gate_2x2
    mat0 = ops_0[0]
    for op in ops_0[1:]: mat0 = np.kron(mat0, op)
    mat1 = ops_1[0]
    for op in ops_1[1:]: mat1 = np.kron(mat1, op)
    return mat0 + mat1

def _probs(sv):
    return (np.abs(sv)**2).tolist()

def _entropy(sv, n_qubits, bipartition=0):
    """Von Neumann entropy via partial trace."""
    dimA = 2 ** (bipartition + 1)
    dimB = 2 ** (n_qubits - bipartition - 1)
    if dimB == 0: dimB = 1
    rho = sv.reshape(dimA, dimB)
    rho_A = rho @ rho.conj().T  # partial trace over B
    eigvals = np.linalg.eigvalsh(rho_A).real
    eigvals = eigvals[eigvals > 1e-12]
    return float(-np.sum(eigvals * np.log2(eigvals))) if len(eigvals) > 0 else 0.0

def _step(sv, n_qubits, gate_name, target, control, narrative, math=""):
    return {
        "gate": gate_name,
        "target": target,
        "control": control,
        "probs": _probs(sv),
        "entropy": _entropy(sv, n_qubits),
        "statevector": [{"re": float(a.real), "im": float(a.imag)} for a in sv],
        "narrative": narrative,
        "math": math,
    }

# ──────────────────────────────────────────────────────────────────
#  Protocol builders (Python)
# ──────────────────────────────────────────────────────────────────

def build_bell_state():
    n = 2
    sv = np.zeros(4, dtype=complex); sv[0] = 1.0
    steps = []

    steps.append(_step(sv.copy(), n, "INIT", -1, -1,
        "Both qubits start in |0⟩. The system is in state |00⟩ with 100% probability. "
        "No entanglement yet — these are two completely independent bits.",
        "|ψ⟩ = |00⟩"))

    sv = _kron_gate(GATE_H, 0, n) @ sv
    steps.append(_step(sv.copy(), n, "H", 0, -1,
        "Hadamard on Alice's qubit creates superposition: Alice is simultaneously |0⟩ and |1⟩. "
        "Bob is still |0⟩. No entanglement yet — just local superposition.",
        "|ψ⟩ = (|0⟩+|1⟩)/√2 ⊗ |0⟩"))

    sv = _controlled_unitary(GATE_X, 0, 1, n) @ sv
    steps.append(_step(sv.copy(), n, "CNOT", 1, 0,
        "CNOT: Bob's qubit flips if Alice's is |1⟩. Because Alice was in superposition, "
        "the result is the Bell state |Φ+⟩ = (|00⟩+|11⟩)/√2 — maximum entanglement. "
        "Measuring one instantly determines the other.",
        "|Φ+⟩ = (|00⟩+|11⟩)/√2"))

    return {
        "name": "Bell State",
        "description": "Two qubits become maximally entangled. The foundation of quantum "
                       "teleportation, superdense coding, and quantum key distribution.",
        "n_qubits": n,
        "qubit_labels": ["Alice", "Bob"],
        "steps": [{**s, "index": i, "total": len(steps)} for i, s in enumerate(steps)]
    }

def build_teleportation():
    n = 3
    sv = np.zeros(8, dtype=complex)
    sv[0] = INV_SQRT2; sv[4] = INV_SQRT2  # |ψ⟩⊗|00⟩, message = (|0⟩+|1⟩)/√2
    steps = []

    steps.append(_step(sv.copy(), n, "INIT", -1, -1,
        "Alice holds an unknown qubit |ψ⟩=(|0⟩+|1⟩)/√2 she wants Bob to receive. "
        "Alice and Bob share a pre-entangled Bell pair (qubits 1 and 2). "
        "Goal: transfer |ψ⟩ to Bob using only 2 classical bits.",
        "|ψ⟩⊗|00⟩"))

    sv = _kron_gate(GATE_H, 1, n) @ sv
    steps.append(_step(sv.copy(), n, "H", 1, -1,
        "Create the Bell pair: Hadamard on qubit 1 (Alice's half of shared pair). "
        "Prepares for entanglement between Alice and Bob.",
        "H on q1"))

    sv = _controlled_unitary(GATE_X, 1, 2, n) @ sv
    steps.append(_step(sv.copy(), n, "CNOT", 2, 1,
        "CNOT entangles Alice (q1) and Bob (q2) into a Bell pair. "
        "Bob now travels far away — this entanglement is the quantum channel.",
        "(|00⟩+|11⟩)/√2 for q1,q2"))

    sv = _controlled_unitary(GATE_X, 0, 1, n) @ sv
    steps.append(_step(sv.copy(), n, "CNOT", 1, 0,
        "Alice starts her Bell measurement: CNOT between message (q0) and her qubit (q1). "
        "This begins encoding the message's information into the entangled system.",
        "Alice's Bell measurement, part 1"))

    sv = _kron_gate(GATE_H, 0, n) @ sv
    steps.append(_step(sv.copy(), n, "H", 0, -1,
        "Alice applies Hadamard to her message qubit. Bell measurement is complete. "
        "Alice measures her 2 qubits (q0, q1) and gets 2 classical bits (00, 01, 10, or 11). "
        "She sends these bits to Bob over a classical channel.",
        "Alice measures → 2 classical bits sent to Bob"))

    sv = _controlled_unitary(GATE_X, 1, 2, n) @ sv
    steps.append(_step(sv.copy(), n, "X-correction", 2, 1,
        "Bob applies corrections based on Alice's classical bits. "
        "After corrections, Bob's qubit (q2) is in exactly the state |ψ⟩ Alice started with. "
        "Teleportation complete! The quantum state moved without physically moving the qubit.",
        "Bob's q2 = Alice's original |ψ⟩ ✓"))

    return {
        "name": "Quantum Teleportation",
        "description": "Transfer an unknown quantum state using entanglement + 2 classical bits.",
        "n_qubits": n,
        "qubit_labels": ["Message (q0)", "Alice (q1)", "Bob (q2)"],
        "steps": [{**s, "index": i, "total": len(steps)} for i, s in enumerate(steps)]
    }

def build_grover():
    n = 2
    sv = np.zeros(4, dtype=complex); sv[0] = 1.0
    steps = []

    steps.append(_step(sv.copy(), n, "INIT", -1, -1,
        "Start: |00⟩. Goal: find marked item |11⟩ in a 4-item database. "
        "Classical worst case: 4 queries. Grover: 1 oracle query for N=4. "
        "For N items: O(√N) quantum vs O(N) classical.",
        "|ψ⟩ = |00⟩"))

    sv = _kron_gate(GATE_H, 0, n) @ sv
    sv = _kron_gate(GATE_H, 1, n) @ sv
    steps.append(_step(sv.copy(), n, "H⊗H", 0, -1,
        "Hadamard on both qubits: uniform superposition over all 4 states. "
        "Each |00⟩, |01⟩, |10⟩, |11⟩ has 25% probability. "
        "The computer considers all 4 database entries simultaneously.",
        "(|00⟩+|01⟩+|10⟩+|11⟩)/2"))

    sv[3] = -sv[3]  # Oracle: phase flip |11⟩
    steps.append(_step(sv.copy(), n, "Oracle", -1, -1,
        "Oracle marks target |11⟩ by flipping its phase (amplitude sign). "
        "This is one quantum query — it acts on all superposed states simultaneously. "
        "Probabilities look unchanged, but the phase difference is now encoded.",
        "(|00⟩+|01⟩+|10⟩−|11⟩)/2"))

    # Diffusion operator: H⊗H · (2|00⟩⟨00|−I) · H⊗H
    sv = _kron_gate(GATE_H, 0, n) @ sv
    sv = _kron_gate(GATE_H, 1, n) @ sv
    sv_new = -sv.copy()
    sv_new[0] += 2 * sv[0]  # add back 2*|00⟩ component
    sv = sv_new
    sv = _kron_gate(GATE_H, 0, n) @ sv
    sv = _kron_gate(GATE_H, 1, n) @ sv
    steps.append(_step(sv.copy(), n, "Diffusion", -1, -1,
        "Grover diffusion operator performs 'inversion about the mean': "
        "amplitudes below average get boosted, above average get suppressed. "
        "Result: |11⟩ now has ~100% probability. One measurement finds the answer. "
        "Quadratic speedup: 1 query instead of up to 4 classically.",
        "|ψ⟩ ≈ |11⟩ — answer found in 1 query!"))

    return {
        "name": "Grover's Search",
        "description": "Find a marked item in an N-element database using O(√N) queries vs O(N) classical.",
        "n_qubits": n,
        "qubit_labels": ["q0", "q1"],
        "speedup": {"classical": "O(N)", "quantum": "O(√N)", "example": "4 items → 1 query"},
        "steps": [{**s, "index": i, "total": len(steps)} for i, s in enumerate(steps)]
    }

def build_deutsch_jozsa():
    n = 2
    sv = np.zeros(4, dtype=complex); sv[1] = 1.0  # |01⟩
    steps = []

    steps.append(_step(sv.copy(), n, "INIT", -1, -1,
        "Start: |0⟩|1⟩. Is f constant (always 0 or always 1) or balanced "
        "(half 0s, half 1s)? Classically: up to N/2+1 queries needed. "
        "Quantum: always exactly 1 query. First-ever proven quantum speedup (1992).",
        "|ψ⟩ = |0⟩|1⟩"))

    sv = _kron_gate(GATE_H, 0, n) @ sv
    sv = _kron_gate(GATE_H, 1, n) @ sv
    steps.append(_step(sv.copy(), n, "H⊗H", 0, -1,
        "Hadamard on both qubits. The ancilla enters |−⟩ state, enabling 'phase kickback': "
        "instead of f(x) being written as an output bit, it will be encoded as a phase. "
        "This is the key quantum trick that enables the 1-query solution.",
        "(|0⟩+|1⟩)|−⟩/√2"))

    sv = _controlled_unitary(GATE_X, 0, 1, n) @ sv  # f(x)=x, balanced
    steps.append(_step(sv.copy(), n, "Oracle (balanced f)", 1, 0,
        "Oracle encodes f(x)=x (balanced: f(0)=0, f(1)=1) via phase kickback. "
        "The input qubit acquires phase (−1)^f(x). No new output qubit needed — "
        "the information is now in the phase of the amplitude.",
        "|x⟩|−⟩ → (−1)^f(x)|x⟩|−⟩"))

    sv = _kron_gate(GATE_H, 0, n) @ sv
    steps.append(_step(sv.copy(), n, "H (readout)", 0, -1,
        "Final Hadamard on input qubit. CONSTANT f → q0 = |0⟩ (100%). "
        "BALANCED f → q0 = |1⟩ (100%). This function is balanced, so we measure |1⟩. "
        "Definitive answer in exactly 1 query, provably impossible classically.",
        "Measure |0⟩ → constant | Measure |1⟩ → balanced"))

    return {
        "name": "Deutsch-Jozsa",
        "description": "Distinguish constant from balanced functions in 1 query. The first proven quantum speedup.",
        "n_qubits": n,
        "qubit_labels": ["Input (q0)", "Ancilla (q1)"],
        "steps": [{**s, "index": i, "total": len(steps)} for i, s in enumerate(steps)]
    }

# ──────────────────────────────────────────────────────────────────
#  Protocol registry
# ──────────────────────────────────────────────────────────────────

BUILDERS = {
    "bell":        build_bell_state,
    "teleport":    build_teleportation,
    "grover":      build_grover,
    "deutsch":     build_deutsch_jozsa,
}

def _call_cpp_engine(name: str) -> dict:
    # run the C++ binary as a subprocess, capture what it prints to stdout.
    # timeout=5 means we give up after 5 seconds (prevents hanging).
    result = subprocess.run(
        [_ENGINE_BIN, name],
        capture_output=True, text=True, timeout=5
    )
    if result.returncode != 0:
        raise RuntimeError(f"C++ engine error: {result.stderr.strip()}")
    # the binary prints a JSON string — parse it into a Python dict
    return json.loads(result.stdout)


@lru_cache(maxsize=None)
def _get_protocol(name):
    # lru_cache means we only simulate each protocol once — results are cached.
    # try C++ engine first; fall back to pure-Python if binary isn't available.
    if USE_CPP:
        try:
            return _call_cpp_engine(name)
        except Exception as e:
            print(f"[backend] C++ engine failed ({e}), falling back to Python")
    builder = BUILDERS.get(name)
    if builder is None:
        return None
    return builder()

# ──────────────────────────────────────────────────────────────────
#  Routes
# ──────────────────────────────────────────────────────────────────

@app.route("/api/protocols")
def list_protocols():
    summaries = []
    for key, builder in BUILDERS.items():
        p = _get_protocol(key)
        summaries.append({
            "id":          key,
            "name":        p["name"],
            "description": p["description"],
            "n_qubits":    p["n_qubits"],
            "n_steps":     len(p["steps"]),
            "qubit_labels": p["qubit_labels"],
        })
    return jsonify(summaries)

@app.route("/api/protocol/<name>")
def get_protocol(name):
    p = _get_protocol(name)
    if p is None:
        abort(404, description=f"Protocol '{name}' not found. Available: {list(BUILDERS.keys())}")
    return jsonify(p)

@app.route("/api/protocol/<name>/step/<int:step_idx>")
def get_step(name, step_idx):
    p = _get_protocol(name)
    if p is None:
        abort(404)
    steps = p["steps"]
    if step_idx < 0 or step_idx >= len(steps):
        abort(400, description=f"Step {step_idx} out of range [0, {len(steps)-1}]")
    return jsonify(steps[step_idx])

@app.route("/api/health")
def health():
    # "engine" tells the frontend whether C++ or Python is doing the simulation.
    # the frontend will display this as a badge so you can see the stack is live.
    return jsonify({
        "status": "ok",
        "protocols": list(BUILDERS.keys()),
        "engine": "cpp" if USE_CPP else "python"
    })

if __name__ == "__main__":
    # port 5000 conflicts with macOS AirPlay Receiver — use 8080 instead.
    PORT = 8080
    print(f"QVis backend starting on http://localhost:{PORT}")
    print("Protocols available:", list(BUILDERS.keys()))
    print("Engine:", "C++ (qvis_engine)" if USE_CPP else "Python (fallback)")
    # use_reloader=False prevents Flask from starting a second watcher process,
    # which makes background testing and subprocess usage simpler.
    app.run(debug=False, port=PORT, use_reloader=False)
