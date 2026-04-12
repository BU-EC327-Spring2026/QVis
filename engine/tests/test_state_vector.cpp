#include <gtest/gtest.h>
#include "state_vector.h"

TEST(StateVectorTest, InitToZeroState) {
    qvis::StateVector sv(2); // 2-qubit system

    EXPECT_EQ(sv.num_qubits(), 2u);
    EXPECT_EQ(sv.dimension(), 4u);

    // |00⟩ should have amplitude 1
    EXPECT_DOUBLE_EQ(sv[0].real(), 1.0);
    EXPECT_DOUBLE_EQ(sv[0].imag(), 0.0);

    // All other basis states should have amplitude 0
    for (std::size_t i = 1; i < sv.dimension(); ++i) {
        EXPECT_DOUBLE_EQ(sv[i].real(), 0.0);
        EXPECT_DOUBLE_EQ(sv[i].imag(), 0.0);
    }
}

TEST(StateVectorTest, SingleQubit) {
    qvis::StateVector sv(1);

    EXPECT_EQ(sv.num_qubits(), 1u);
    EXPECT_EQ(sv.dimension(), 2u);
    EXPECT_DOUBLE_EQ(sv[0].real(), 1.0);
    EXPECT_DOUBLE_EQ(sv[1].real(), 0.0);
}

TEST(StateVectorTest, MutableAccess) {
    qvis::StateVector sv(1);

    // Manually set to |1⟩
    sv[0] = {0.0, 0.0};
    sv[1] = {1.0, 0.0};

    EXPECT_DOUBLE_EQ(sv[0].real(), 0.0);
    EXPECT_DOUBLE_EQ(sv[1].real(), 1.0);
}
