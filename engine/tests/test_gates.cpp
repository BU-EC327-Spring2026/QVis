#include <cmath>
#include <gtest/gtest.h>
#include "state_vector.h"
#include "gates.h"

static const double INV_SQRT2 = 1.0 / std::sqrt(2.0);

// H|0⟩ = (|0⟩ + |1⟩) / √2
TEST(GatesTest, HadamardOnZero) {
    qvis::StateVector sv(1);
    sv.apply(qvis::hadamard(), 0);

    EXPECT_NEAR(sv[0].real(), INV_SQRT2, 1e-12);
    EXPECT_NEAR(sv[0].imag(), 0.0, 1e-12);
    EXPECT_NEAR(sv[1].real(), INV_SQRT2, 1e-12);
    EXPECT_NEAR(sv[1].imag(), 0.0, 1e-12);
}

// H|1⟩ = (|0⟩ - |1⟩) / √2
TEST(GatesTest, HadamardOnOne) {
    qvis::StateVector sv(1);
    // Prepare |1⟩
    sv[0] = {0.0, 0.0};
    sv[1] = {1.0, 0.0};

    sv.apply(qvis::hadamard(), 0);

    EXPECT_NEAR(sv[0].real(), INV_SQRT2, 1e-12);
    EXPECT_NEAR(sv[0].imag(), 0.0, 1e-12);
    EXPECT_NEAR(sv[1].real(), -INV_SQRT2, 1e-12);
    EXPECT_NEAR(sv[1].imag(), 0.0, 1e-12);
}

// HH = I: applying Hadamard twice returns to original state
TEST(GatesTest, HadamardTwiceIsIdentity) {
    qvis::StateVector sv(1);
    sv.apply(qvis::hadamard(), 0);
    sv.apply(qvis::hadamard(), 0);

    EXPECT_NEAR(sv[0].real(), 1.0, 1e-12);
    EXPECT_NEAR(sv[0].imag(), 0.0, 1e-12);
    EXPECT_NEAR(sv[1].real(), 0.0, 1e-12);
    EXPECT_NEAR(sv[1].imag(), 0.0, 1e-12);
}

// 2-qubit: H on qubit 0 of |00⟩ gives (|00⟩ + |01⟩) / √2
// Little-endian: |00⟩ = index 0, |01⟩ = index 1
TEST(GatesTest, HadamardQubit0_TwoQubitSystem) {
    qvis::StateVector sv(2);
    sv.apply(qvis::hadamard(), 0);

    EXPECT_NEAR(sv[0].real(), INV_SQRT2, 1e-12);  // |00⟩
    EXPECT_NEAR(sv[1].real(), INV_SQRT2, 1e-12);  // |01⟩
    EXPECT_NEAR(sv[2].real(), 0.0, 1e-12);         // |10⟩
    EXPECT_NEAR(sv[3].real(), 0.0, 1e-12);         // |11⟩

    // All imaginary parts should be 0
    for (std::size_t i = 0; i < sv.dimension(); ++i) {
        EXPECT_NEAR(sv[i].imag(), 0.0, 1e-12);
    }
}

// ---- X (Pauli-X / NOT) gate ------------------------------------------------

// X|0⟩ = |1⟩
TEST(GatesTest, PauliXOnZero) {
    qvis::StateVector sv(1);
    sv.apply(qvis::pauli_x(), 0);

    EXPECT_NEAR(sv[0].real(), 0.0, 1e-12);
    EXPECT_NEAR(sv[1].real(), 1.0, 1e-12);
}

// X|1⟩ = |0⟩
TEST(GatesTest, PauliXOnOne) {
    qvis::StateVector sv(1);
    sv[0] = {0.0, 0.0};
    sv[1] = {1.0, 0.0};
    sv.apply(qvis::pauli_x(), 0);

    EXPECT_NEAR(sv[0].real(), 1.0, 1e-12);
    EXPECT_NEAR(sv[1].real(), 0.0, 1e-12);
}

// XX = I: applying X twice returns to original state
TEST(GatesTest, PauliXTwiceIsIdentity) {
    qvis::StateVector sv(1);
    sv.apply(qvis::pauli_x(), 0);
    sv.apply(qvis::pauli_x(), 0);

    EXPECT_NEAR(sv[0].real(), 1.0, 1e-12);
    EXPECT_NEAR(sv[1].real(), 0.0, 1e-12);
}

// 2-qubit: X on qubit 1 of |00⟩ → |10⟩ (index 2)
TEST(GatesTest, PauliXQubit1_TwoQubit) {
    qvis::StateVector sv(2);
    sv.apply(qvis::pauli_x(), 1);

    EXPECT_NEAR(sv[0].real(), 0.0, 1e-12);  // |00⟩
    EXPECT_NEAR(sv[1].real(), 0.0, 1e-12);  // |01⟩
    EXPECT_NEAR(sv[2].real(), 1.0, 1e-12);  // |10⟩
    EXPECT_NEAR(sv[3].real(), 0.0, 1e-12);  // |11⟩
}

// ---- CNOT gate --------------------------------------------------------------

// CNOT with control=0, target=1 on |00⟩ → |00⟩ (control is 0, no flip)
TEST(GatesTest, CNOTControlZero_NoFlip) {
    qvis::StateVector sv(2);
    qvis::cnot(sv, 0, 1);

    EXPECT_NEAR(sv[0].real(), 1.0, 1e-12);  // |00⟩
    EXPECT_NEAR(sv[1].real(), 0.0, 1e-12);
    EXPECT_NEAR(sv[2].real(), 0.0, 1e-12);
    EXPECT_NEAR(sv[3].real(), 0.0, 1e-12);
}

// CNOT with control=0, target=1 on |01⟩ → |11⟩
// |01⟩ = index 1 (qubit 0 = 1), control qubit 0 is |1⟩ → flip qubit 1
TEST(GatesTest, CNOTControlOne_FlipsTarget) {
    qvis::StateVector sv(2);
    sv[0] = {0.0, 0.0};
    sv[1] = {1.0, 0.0};  // |01⟩

    qvis::cnot(sv, 0, 1);

    // |01⟩ → |11⟩ = index 3
    EXPECT_NEAR(sv[0].real(), 0.0, 1e-12);
    EXPECT_NEAR(sv[1].real(), 0.0, 1e-12);
    EXPECT_NEAR(sv[2].real(), 0.0, 1e-12);
    EXPECT_NEAR(sv[3].real(), 1.0, 1e-12);
}

// Bell state: H on qubit 0, then CNOT(0,1) on |00⟩ → (|00⟩ + |11⟩)/√2
TEST(GatesTest, BellState) {
    qvis::StateVector sv(2);
    sv.apply(qvis::hadamard(), 0);  // (|00⟩ + |01⟩)/√2
    qvis::cnot(sv, 0, 1);           // (|00⟩ + |11⟩)/√2

    EXPECT_NEAR(sv[0].real(), INV_SQRT2, 1e-12);  // |00⟩
    EXPECT_NEAR(sv[1].real(), 0.0, 1e-12);         // |01⟩
    EXPECT_NEAR(sv[2].real(), 0.0, 1e-12);         // |10⟩
    EXPECT_NEAR(sv[3].real(), INV_SQRT2, 1e-12);  // |11⟩
}

// CNOT is its own inverse: applying twice returns to original
TEST(GatesTest, CNOTTwiceIsIdentity) {
    qvis::StateVector sv(2);
    sv[0] = {0.0, 0.0};
    sv[1] = {1.0, 0.0};  // |01⟩

    qvis::cnot(sv, 0, 1);
    qvis::cnot(sv, 0, 1);

    EXPECT_NEAR(sv[0].real(), 0.0, 1e-12);
    EXPECT_NEAR(sv[1].real(), 1.0, 1e-12);  // back to |01⟩
    EXPECT_NEAR(sv[2].real(), 0.0, 1e-12);
    EXPECT_NEAR(sv[3].real(), 0.0, 1e-12);
}
