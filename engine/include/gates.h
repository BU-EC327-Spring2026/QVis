#ifndef QVIS_GATES_H
#define QVIS_GATES_H

#include "state_vector.h"
#include <cmath>

namespace qvis {

/// Returns the 2x2 Hadamard matrix: (1/√2) * [[1, 1], [1, -1]]
inline StateVector::Gate2x2 hadamard() {
    const double r = 1.0 / std::sqrt(2.0);
    return {{
        {{{r, 0.0}, {r, 0.0}}},
        {{{r, 0.0}, {-r, 0.0}}}
    }};
}

/// Returns the 2x2 Pauli-X (NOT) matrix: [[0, 1], [1, 0]]
inline StateVector::Gate2x2 pauli_x() {
    return {{
        {{{0.0, 0.0}, {1.0, 0.0}}},
        {{{1.0, 0.0}, {0.0, 0.0}}}
    }};
}

/// Apply a CNOT gate: flips target qubit when control qubit is |1⟩.
inline void cnot(StateVector& sv, std::size_t control, std::size_t target) {
    sv.apply_controlled(pauli_x(), control, target);
}

} // namespace qvis

#endif // QVIS_GATES_H
