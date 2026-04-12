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

const StateVector::Amplitude* StateVector::data() const {
    return amplitudes_.data();
}

StateVector::Amplitude* StateVector::data() { return amplitudes_.data(); }

} // namespace qvis
