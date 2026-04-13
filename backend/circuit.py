"""Circuit execution: walks a gate list and applies operations to a StateVector."""

from bindings import StateVector
from bindings.qvis_engine_ext import hadamard, pauli_x, cnot


def _apply_h(sv, targets):
    matrix = hadamard()
    for t in targets:
        sv.apply(matrix, t)


def _apply_x(sv, targets):
    matrix = pauli_x()
    for t in targets:
        sv.apply(matrix, t)


def _apply_cnot(sv, targets):
    if len(targets) != 2:
        raise ValueError("CNOT requires exactly 2 targets: [control, target]")
    cnot(sv, targets[0], targets[1])


# Maps gate name -> callable(sv, targets)
GATE_REGISTRY = {
    "h": _apply_h,
    "x": _apply_x,
    "cx": _apply_cnot,
    "cnot": _apply_cnot,
}


def max_qubit(circuit):
    """Return the highest qubit index referenced in the circuit."""
    m = 0
    for op in circuit:
        for t in op["targets"]:
            if t > m:
                m = t
    return m


def run_circuit(circuit, num_qubits=None):
    """Execute a circuit (list of {"gate", "targets"} dicts) on a fresh StateVector.

    Returns the final StateVector.
    """
    if num_qubits is None:
        num_qubits = max_qubit(circuit) + 1 if circuit else 1

    sv = StateVector(num_qubits)

    for op in circuit:
        gate_fn = GATE_REGISTRY.get(op["gate"])
        if gate_fn is None:
            raise ValueError(f"Unknown gate: {op['gate']!r}")
        gate_fn(sv, op["targets"])

    return sv


def run_circuit_with_steps(circuit, num_qubits=None):
    """Execute a circuit, capturing state snapshots after each gate.

    Returns (sv, steps) where steps is a list of (label, sv_snapshot) tuples.
    The sv_snapshot is a list of complex amplitudes at that point.
    """
    if num_qubits is None:
        num_qubits = max_qubit(circuit) + 1 if circuit else 1

    sv = StateVector(num_qubits)

    def snapshot():
        return [sv[i] for i in range(sv.dimension())]

    steps = [("Initial state", snapshot())]

    for op in circuit:
        gate_fn = GATE_REGISTRY.get(op["gate"])
        if gate_fn is None:
            raise ValueError(f"Unknown gate: {op['gate']!r}")
        gate_fn(sv, op["targets"])

        name = op["gate"].upper()
        qubits = ", ".join(str(t) for t in op["targets"])
        if len(op["targets"]) == 1:
            label = f"{name} on qubit {qubits}"
        else:
            label = f"{name} on qubits {qubits}"

        steps.append((label, snapshot()))

    return sv, steps
