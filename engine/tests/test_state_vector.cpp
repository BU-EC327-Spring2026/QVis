#include <cmath>
#include <gtest/gtest.h>
#include "state_vector.h"

// Convenience gate matrices.
// Little-endian qubit ordering: bit 0 = qubit 0 (least significant).

static const qvis::StateVector::Gate2x2 I_GATE = {{
    {{{1.0, 0.0}, {0.0, 0.0}}},
    {{{0.0, 0.0}, {1.0, 0.0}}}
}};

static const qvis::StateVector::Gate2x2 X_GATE = {{
    {{{0.0, 0.0}, {1.0, 0.0}}},
    {{{1.0, 0.0}, {0.0, 0.0}}}
}};

// --- Initialization tests ----------------------------------------------------

TEST(StateVectorTest, InitToZeroState_1Qubit) {
    qvis::StateVector sv(1);
    EXPECT_DOUBLE_EQ(sv[0].real(), 1.0);
    EXPECT_DOUBLE_EQ(sv[0].imag(), 0.0);
    EXPECT_DOUBLE_EQ(sv[1].real(), 0.0);
}

TEST(StateVectorTest, InitToZeroState_2Qubit) {
    qvis::StateVector sv(2);
    EXPECT_DOUBLE_EQ(sv[0].real(), 1.0);
    for (std::size_t i = 1; i < sv.dimension(); ++i) {
        EXPECT_DOUBLE_EQ(sv[i].real(), 0.0);
        EXPECT_DOUBLE_EQ(sv[i].imag(), 0.0);
    }
}

TEST(StateVectorTest, InitToZeroState_3Qubit) {
    qvis::StateVector sv(3);
    EXPECT_DOUBLE_EQ(sv[0].real(), 1.0);
    for (std::size_t i = 1; i < sv.dimension(); ++i) {
        EXPECT_DOUBLE_EQ(sv[i].real(), 0.0);
    }
}

// --- Dimension tests ---------------------------------------------------------

TEST(StateVectorTest, CorrectDimension) {
    for (std::size_t n = 1; n <= 5; ++n) {
        qvis::StateVector sv(n);
        EXPECT_EQ(sv.num_qubits(), n);
        EXPECT_EQ(sv.dimension(), static_cast<std::size_t>(1) << n);
    }
}

// --- Identity gate tests -----------------------------------------------------

TEST(StateVectorTest, IdentityPreservesState_1Qubit) {
    qvis::StateVector sv(1);
    sv.apply(I_GATE, 0);

    EXPECT_DOUBLE_EQ(sv[0].real(), 1.0);
    EXPECT_DOUBLE_EQ(sv[1].real(), 0.0);
}

TEST(StateVectorTest, IdentityPreservesState_2Qubit_AllQubits) {
    qvis::StateVector sv(2);
    sv.apply(I_GATE, 0);
    sv.apply(I_GATE, 1);

    EXPECT_DOUBLE_EQ(sv[0].real(), 1.0);
    for (std::size_t i = 1; i < sv.dimension(); ++i) {
        EXPECT_DOUBLE_EQ(sv[i].real(), 0.0);
    }
}

// --- Apply gate tests --------------------------------------------------------

// 2-qubit system, little-endian:
//   index 0 = |00⟩, index 1 = |01⟩, index 2 = |10⟩, index 3 = |11⟩
// X on qubit 0 flips bit 0:  |00⟩ → |01⟩  (index 0 → index 1)
// X on qubit 1 flips bit 1:  |00⟩ → |10⟩  (index 0 → index 2)

TEST(StateVectorTest, ApplyXGateQubit0) {
    qvis::StateVector sv(2);
    sv.apply(X_GATE, 0);

    // Should be |01⟩ = index 1
    EXPECT_DOUBLE_EQ(sv[0].real(), 0.0);
    EXPECT_DOUBLE_EQ(sv[1].real(), 1.0);
    EXPECT_DOUBLE_EQ(sv[2].real(), 0.0);
    EXPECT_DOUBLE_EQ(sv[3].real(), 0.0);
}

TEST(StateVectorTest, ApplyXGateQubit1) {
    qvis::StateVector sv(2);
    sv.apply(X_GATE, 1);

    // Should be |10⟩ = index 2
    EXPECT_DOUBLE_EQ(sv[0].real(), 0.0);
    EXPECT_DOUBLE_EQ(sv[1].real(), 0.0);
    EXPECT_DOUBLE_EQ(sv[2].real(), 1.0);
    EXPECT_DOUBLE_EQ(sv[3].real(), 0.0);
}

TEST(StateVectorTest, ApplyXToBothQubitsIndependently) {
    qvis::StateVector sv(2);
    sv.apply(X_GATE, 0);  // |00⟩ → |01⟩
    sv.apply(X_GATE, 1);  // |01⟩ → |11⟩

    // Should be |11⟩ = index 3
    EXPECT_DOUBLE_EQ(sv[0].real(), 0.0);
    EXPECT_DOUBLE_EQ(sv[1].real(), 0.0);
    EXPECT_DOUBLE_EQ(sv[2].real(), 0.0);
    EXPECT_DOUBLE_EQ(sv[3].real(), 1.0);
}
