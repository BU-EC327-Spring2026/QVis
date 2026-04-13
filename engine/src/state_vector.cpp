#include "state_vector.h"

namespace qvis {

StateVector::StateVector(std::size_t num_qubits)
    : num_qubits_(num_qubits),
      amplitudes_(static_cast<std::size_t>(1) << num_qubits, {0.0, 0.0}) {
    // Initialise to |0...0⟩
    amplitudes_[0] = {1.0, 0.0};
}

std::size_t StateVector::num_qubits() const { return num_qubits_; }

std::size_t StateVector::dimension() const { return amplitudes_.size(); }

const StateVector::Amplitude& StateVector::operator[](std::size_t index) const {
    return amplitudes_[index];
}

StateVector::Amplitude& StateVector::operator[](std::size_t index) {
    return amplitudes_[index];
}

void StateVector::apply(const Gate2x2& m, std::size_t target_qubit) {
    const std::size_t dim = dimension();
    const std::size_t bit = static_cast<std::size_t>(1) << target_qubit;

    for (std::size_t i = 0; i < dim; ++i) {
        // Only process pairs once: when the target qubit bit is 0.
        if (i & bit) continue;

        std::size_t i0 = i;        // target qubit = 0
        std::size_t i1 = i | bit;  // target qubit = 1

        Amplitude a0 = amplitudes_[i0];
        Amplitude a1 = amplitudes_[i1];

        amplitudes_[i0] = m[0][0] * a0 + m[0][1] * a1;
        amplitudes_[i1] = m[1][0] * a0 + m[1][1] * a1;
    }
}

void StateVector::apply_controlled(const Gate2x2& m, std::size_t control_qubit,
                                   std::size_t target_qubit) {
    const std::size_t dim = dimension();
    const std::size_t ctrl_bit = static_cast<std::size_t>(1) << control_qubit;
    const std::size_t tgt_bit = static_cast<std::size_t>(1) << target_qubit;

    for (std::size_t i = 0; i < dim; ++i) {
        // Only process when control bit is 1 and target bit is 0
        // (to visit each affected pair exactly once).
        if (!(i & ctrl_bit)) continue;
        if (i & tgt_bit) continue;

        std::size_t i0 = i;          // target qubit = 0, control = 1
        std::size_t i1 = i | tgt_bit; // target qubit = 1, control = 1

        Amplitude a0 = amplitudes_[i0];
        Amplitude a1 = amplitudes_[i1];

        amplitudes_[i0] = m[0][0] * a0 + m[0][1] * a1;
        amplitudes_[i1] = m[1][0] * a0 + m[1][1] * a1;
    }
}

const StateVector::Amplitude* StateVector::data() const {
    return amplitudes_.data();
}

StateVector::Amplitude* StateVector::data() { return amplitudes_.data(); }

} // namespace qvis
