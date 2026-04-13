#ifndef QVIS_STATE_VECTOR_H
#define QVIS_STATE_VECTOR_H

#include <array>
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
    using Gate2x2 = std::array<std::array<Amplitude, 2>, 2>;

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

    /// Apply a 2x2 gate matrix to the given target qubit.
    /// Uses the tensor product structure: iterates over basis state pairs
    /// that differ only in the target qubit bit.
    void apply(const Gate2x2& matrix, std::size_t target_qubit);

    /// Apply a 2x2 gate matrix to the target qubit, controlled by the
    /// control qubit. The gate is only applied to basis states where the
    /// control qubit is |1⟩.
    void apply_controlled(const Gate2x2& matrix, std::size_t control_qubit,
                          std::size_t target_qubit);

    /// Raw pointer to amplitudes (for engine internals).
    const Amplitude* data() const;
    Amplitude* data();

private:
    std::size_t num_qubits_;
    std::vector<Amplitude> amplitudes_;
};

} // namespace qvis

#endif // QVIS_STATE_VECTOR_H
