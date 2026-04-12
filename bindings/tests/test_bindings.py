import math
from bindings import StateVector
from bindings.qvis_engine_ext import hadamard, sample


def test_statevector_init():
    sv = StateVector(1)
    assert len(sv) == 2
    assert sv[0] == complex(1, 0)
    assert sv[1] == complex(0, 0)


def test_hadamard_amplitudes():
    sv = StateVector(1)
    sv.apply(hadamard(), 0)

    expected = 1.0 / math.sqrt(2.0)
    assert abs(sv[0].real - expected) < 1e-12
    assert abs(sv[0].imag) < 1e-12
    assert abs(sv[1].real - expected) < 1e-12
    assert abs(sv[1].imag) < 1e-12


def test_sample_hadamard():
    sv = StateVector(1)
    sv.apply(hadamard(), 0)

    counts = sample(sv, 1000)
    assert "0" in counts
    assert "1" in counts
    assert counts["0"] + counts["1"] == 1000


def test_bell_state_pipeline():
    sv = StateVector(2)

    # Apply H to qubit 0: produces (|00⟩ + |01⟩) / √2
    sv.apply(hadamard(), 0)

    # Verify intermediate state via sampling
    counts_h = sample(sv, 1000)
    assert set(counts_h.keys()) <= {"00", "01"}
    assert counts_h.get("00", 0) + counts_h.get("01", 0) == 1000

    # Prepare Bell state (|00⟩ + |11⟩) / √2 by direct amplitude writes,
    # since CNOT is not yet implemented.
    r = 1.0 / math.sqrt(2.0)
    sv[0] = complex(r, 0)
    sv[1] = complex(0, 0)
    sv[2] = complex(0, 0)
    sv[3] = complex(r, 0)

    counts_bell = sample(sv, 1000)
    assert "01" not in counts_bell
    assert "10" not in counts_bell
    assert counts_bell.get("00", 0) + counts_bell.get("11", 0) == 1000
