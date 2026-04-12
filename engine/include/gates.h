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

} // namespace qvis

#endif // QVIS_GATES_H
