#ifndef QVIS_MEASUREMENT_H
#define QVIS_MEASUREMENT_H

#include "state_vector.h"
#include <algorithm>
#include <map>
#include <random>
#include <string>
#include <vector>

namespace qvis {

/// Convert a basis state index to a little-endian bitstring.
/// Qubit 0 = rightmost character.
inline std::string index_to_bitstring(std::size_t index, std::size_t num_qubits) {
    std::string bits(num_qubits, '0');
    for (std::size_t q = 0; q < num_qubits; ++q) {
        if (index & (static_cast<std::size_t>(1) << q))
            bits[num_qubits - 1 - q] = '1';
    }
    return bits;
}

/// Sample the state vector `shots` times, returning a map from
/// bitstring outcomes to counts.
inline std::map<std::string, int> sample(const StateVector& sv, int shots) {
    const std::size_t dim = sv.dimension();
    const std::size_t n = sv.num_qubits();

    // Build cumulative probability distribution.
    std::vector<double> cdf(dim);
    double cumulative = 0.0;
    for (std::size_t i = 0; i < dim; ++i) {
        cumulative += std::norm(sv[i]);  // |amplitude|^2
        cdf[i] = cumulative;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    std::map<std::string, int> counts;
    for (int s = 0; s < shots; ++s) {
        double r = dist(gen);
        // Binary search for the first index where cdf[index] > r.
        auto it = std::upper_bound(cdf.begin(), cdf.end(), r);
        std::size_t outcome = static_cast<std::size_t>(it - cdf.begin());
        if (outcome >= dim) outcome = dim - 1;  // guard against floating-point edge case

        counts[index_to_bitstring(outcome, n)]++;
    }

    return counts;
}

} // namespace qvis

#endif // QVIS_MEASUREMENT_H
