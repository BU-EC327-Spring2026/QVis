#include <cmath>
#include <gtest/gtest.h>
#include "state_vector.h"
#include "gates.h"
#include "measurement.h"

// Sampling |0⟩ with 100 shots must return exactly {"0": 100}.
TEST(MeasurementTest, DeterministicZeroState) {
    qvis::StateVector sv(1);
    auto counts = qvis::sample(sv, 100);

    EXPECT_EQ(counts.size(), 1u);
    EXPECT_EQ(counts["0"], 100);
}

// Sampling H|0⟩ with 10000 shots should give roughly 50/50.
TEST(MeasurementTest, HadamardFiftyFifty) {
    qvis::StateVector sv(1);
    sv.apply(qvis::hadamard(), 0);

    auto counts = qvis::sample(sv, 10000);

    EXPECT_EQ(counts.size(), 2u);
    EXPECT_GT(counts["0"], 4000);
    EXPECT_LT(counts["0"], 6000);
    EXPECT_GT(counts["1"], 4000);
    EXPECT_LT(counts["1"], 6000);
}

// Bell state (|00⟩ + |11⟩)/√2 should only produce "00" and "11".
TEST(MeasurementTest, BellStateOnlyCorrelatedOutcomes) {
    qvis::StateVector sv(2);

    // Manually prepare the Bell state since CNOT isn't implemented yet.
    const double r = 1.0 / std::sqrt(2.0);
    sv[0] = {r, 0.0};  // |00⟩
    sv[1] = {0.0, 0.0}; // |01⟩
    sv[2] = {0.0, 0.0}; // |10⟩
    sv[3] = {r, 0.0};  // |11⟩

    auto counts = qvis::sample(sv, 10000);

    // Only "00" and "11" should appear.
    EXPECT_EQ(counts.count("01"), 0u);
    EXPECT_EQ(counts.count("10"), 0u);
    EXPECT_GT(counts["00"], 4000);
    EXPECT_LT(counts["00"], 6000);
    EXPECT_GT(counts["11"], 4000);
    EXPECT_LT(counts["11"], 6000);
}
