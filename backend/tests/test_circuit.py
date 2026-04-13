import math
from backend.circuit import run_circuit, run_circuit_with_steps


def test_empty_circuit():
    sv = run_circuit([])
    assert sv.num_qubits() == 1
    assert abs(sv[0] - complex(1, 0)) < 1e-12


def test_hadamard_circuit():
    sv = run_circuit([{"gate": "h", "targets": [0]}])
    r = 1.0 / math.sqrt(2.0)
    assert abs(sv[0].real - r) < 1e-12
    assert abs(sv[1].real - r) < 1e-12


def test_x_gate():
    sv = run_circuit([{"gate": "x", "targets": [0]}])
    assert abs(sv[0]) < 1e-12
    assert abs(sv[1] - complex(1, 0)) < 1e-12


def test_cnot_no_flip():
    # |00⟩ with CNOT(0,1) → |00⟩ (control qubit 0 is 0)
    sv = run_circuit([{"gate": "cx", "targets": [0, 1]}])
    assert abs(sv[0] - complex(1, 0)) < 1e-12
    assert abs(sv[1]) < 1e-12
    assert abs(sv[2]) < 1e-12
    assert abs(sv[3]) < 1e-12


def test_cnot_with_flip():
    # X on qubit 0, then CNOT(0,1): |00⟩ → |01⟩ → |11⟩
    sv = run_circuit([
        {"gate": "x", "targets": [0]},
        {"gate": "cx", "targets": [0, 1]},
    ])
    assert abs(sv[0]) < 1e-12
    assert abs(sv[1]) < 1e-12
    assert abs(sv[2]) < 1e-12
    assert abs(sv[3] - complex(1, 0)) < 1e-12


def test_bell_state():
    r = 1.0 / math.sqrt(2.0)
    sv = run_circuit([
        {"gate": "h", "targets": [0]},
        {"gate": "cx", "targets": [0, 1]},
    ])
    assert abs(sv[0].real - r) < 1e-12  # |00⟩
    assert abs(sv[1]) < 1e-12            # |01⟩
    assert abs(sv[2]) < 1e-12            # |10⟩
    assert abs(sv[3].real - r) < 1e-12  # |11⟩


def test_cnot_alias():
    # "cnot" should work the same as "cx"
    sv = run_circuit([
        {"gate": "x", "targets": [0]},
        {"gate": "cnot", "targets": [0, 1]},
    ])
    assert abs(sv[3] - complex(1, 0)) < 1e-12


def test_steps_count():
    _, steps = run_circuit_with_steps([
        {"gate": "h", "targets": [0]},
        {"gate": "cx", "targets": [0, 1]},
    ])
    # Initial + 2 gates = 3 steps
    assert len(steps) == 3
    assert steps[0][0] == "Initial state"
    assert steps[1][0] == "H on qubit 0"
    assert steps[2][0] == "CX on qubits 0, 1"


def test_unknown_gate_raises():
    try:
        run_circuit([{"gate": "zzzz", "targets": [0]}])
        assert False, "Should have raised ValueError"
    except ValueError as e:
        assert "Unknown gate" in str(e)
