#ifndef QVIS_STATE_VECTOR_H
#define QVIS_STATE_VECTOR_H

#include <complex>
#include <cstddef>
#include <vector>

namespace qvis {

/// State vector for an n-qubit system.
/// Little-endian qubit ordering: index i corresponds to the binary
/// representation of i where bit 0 = qubit 0 (least significant).
class StateVector {
public:
    using Amplitude = std::complex<double>;

    /// Construct the |0...0⟩ state for n qubits.
    explicit StateVector(std::size_t num_qubits);

    /// Number of qubits in the system.
    std::size_t num_qubits() const;

    /// Dimension of the state vector (2^n).
    std::size_t dimension() const;

    /// Read-only access to amplitude at basis state index.
    const Amplitude& operator[](std::size_t index) const;

    /// Mutable access to amplitude at basis state index.
    Amplitude& operator[](std::size_t index);

    /// Raw pointer to amplitudes (for engine internals).
    const Amplitude* data() const;
    Amplitude* data();

private:
    std::size_t num_qubits_;
    std::vector<Amplitude> amplitudes_;
};

} // namespace qvis

#endif // QVIS_STATE_VECTOR_H
