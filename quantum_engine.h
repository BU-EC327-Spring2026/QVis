#pragma once
#include <complex>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <utility>
#include <cmath>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────
//  QVis Quantum Simulation Engine
//  EC327 Spring 2026 — Team GAM
//
//  Architecture:
//    Statevector: 2^n complex amplitudes stored as std::vector<cx>
//    Gates:       Applied as sparse matrix-vector products
//    Protocols:   Pre-built sequences of GateOps with narrative text
// ─────────────────────────────────────────────────────────────────

namespace qvis {

using cx  = std::complex<double>;
using Vec = std::vector<cx>;
struct Step;

// ── Constants ──────────────────────────────────────────────────
constexpr double INV_SQRT2 = 0.7071067811865475;

// ── Single gate application ────────────────────────────────────
// Applies a 1-qubit gate (2×2 unitary) to qubit `target` in an n-qubit system.
// `gate` is stored row-major: {a,b,c,d} → [[a,b],[c,d]]
void apply_single(Vec& sv, int n_qubits, int target, const cx gate[4]);

// Applies a controlled-1-qubit gate. `control` must differ from `target`.
void apply_controlled(Vec& sv, int n_qubits, int control, int target, const cx gate[4]);

// Applies SWAP gate between two qubits.
void apply_swap(Vec& sv, int n_qubits, int q0, int q1);

// ── Measurements ───────────────────────────────────────────────
// Returns probability of measuring |1⟩ on qubit `q` (does NOT collapse state).
double measure_prob(const Vec& sv, int n_qubits, int q);

// Returns full Born-rule probability distribution over all 2^n basis states.
std::vector<double> probabilities(const Vec& sv);

// ── Entropy ────────────────────────────────────────────────────
// Von Neumann entropy approximated via diagonal of reduced density matrix.
// Ranges 0 (pure/product) to log2(min(dimA,dimB)) (maximally entangled).
double entanglement_entropy(const Vec& sv, int n_qubits, int bipartition_qubit);

// ── Named gate factories ────────────────────────────────────────
struct Gates {
    // Returns gate matrix as 4-element array (caller must hold lifetime)
    static void H(cx out[4]);      // Hadamard
    static void X(cx out[4]);      // Pauli-X (NOT)
    static void Y(cx out[4]);      // Pauli-Y
    static void Z(cx out[4]);      // Pauli-Z
    static void S(cx out[4]);      // Phase gate (√Z)
    static void T(cx out[4]);      // T gate (√S)
    static void Rx(double theta, cx out[4]); // Rotation around X
    static void Rz(double theta, cx out[4]); // Rotation around Z
    static void I(cx out[4]);      // Identity
};

// ── Polymorphic circuit model ───────────────────────────────────
// These classes let protocols store different gate behaviors behind one
// common interface. New gate families can be added by deriving QuantumGate.
class QuantumGate {
public:
    QuantumGate(std::string name, int target, int control,
                std::string narrative, std::string math_hint = "");
    virtual ~QuantumGate() = default;

    virtual void apply(Vec& sv, int n_qubits) const = 0;

    const std::string& name() const;
    int target() const;
    int control() const;
    const std::string& narrative() const;
    const std::string& math_hint() const;

private:
    std::string name_;
    int target_;
    int control_;
    std::string narrative_;
    std::string math_hint_;
};

using GateFactory = std::function<void(cx out[4])>;

class SingleQubitGate : public QuantumGate {
public:
    SingleQubitGate(std::string name, int target, GateFactory factory,
                    std::string narrative, std::string math_hint = "");
    void apply(Vec& sv, int n_qubits) const override;

private:
    GateFactory factory_;
};

class ParallelSingleQubitGate : public QuantumGate {
public:
    ParallelSingleQubitGate(std::string name, std::vector<int> targets,
                            GateFactory factory, std::string narrative,
                            std::string math_hint = "");
    void apply(Vec& sv, int n_qubits) const override;

private:
    std::vector<int> targets_;
    GateFactory factory_;
};

class ControlledGate : public QuantumGate {
public:
    ControlledGate(std::string name, int control, int target, GateFactory factory,
                   std::string narrative, std::string math_hint = "");
    void apply(Vec& sv, int n_qubits) const override;

private:
    GateFactory factory_;
};

class PhaseOracleGate : public QuantumGate {
public:
    PhaseOracleGate(std::string name, int marked_state,
                    std::string narrative, std::string math_hint = "");
    void apply(Vec& sv, int n_qubits) const override;

private:
    int marked_state_;
};

class DiffusionGate : public QuantumGate {
public:
    DiffusionGate(std::string name, std::string narrative,
                  std::string math_hint = "");
    void apply(Vec& sv, int n_qubits) const override;
};

class Circuit {
public:
    Circuit(int n_qubits, Vec initial_state);

    void add_gate(std::unique_ptr<QuantumGate> gate);
    std::vector<Step> run(const std::string& initial_narrative,
                          const std::string& initial_math_hint) const;

private:
    int n_qubits_;
    Vec initial_state_;
    std::vector<std::unique_ptr<QuantumGate>> gates_;
};

// ── Step-through data model ─────────────────────────────────────
// One step = one gate event + snapshot of state + narrative text
struct Step {
    std::string gate_name;      // e.g. "H", "CNOT", "Measure"
    int         qubit_target;   // primary qubit (0-indexed)
    int         qubit_control;  // -1 if not a controlled gate
    Vec         statevector;    // snapshot AFTER this gate
    std::vector<double> probs;  // |amplitude|^2 for each basis state
    double      entropy;        // entanglement entropy at bipartition 0|rest
    std::string narrative;      // human-readable explanation of this step
    std::string math_hint;      // optional LaTeX-style string for UI
};

// ── Protocol runner ─────────────────────────────────────────────
// A protocol is a named sequence of steps, fully pre-computed.
struct Protocol {
    std::string         name;
    std::string         description;
    int                 n_qubits;
    std::vector<Step>   steps;      // steps[0] = initial state (no gate applied)
    std::vector<std::string> qubit_labels; // e.g. {"Alice","Bob","Charlie"}
};

// Build the core protocols. Returns fully populated Protocol.
Protocol make_bell_state();
Protocol make_teleportation();
Protocol make_grover_2qubit();
Protocol make_grover_3qubit();
Protocol make_deutsch_jozsa();

// ── JSON serialization ──────────────────────────────────────────
// Serialize a Protocol to a compact JSON string for the REST endpoint.
std::string protocol_to_json(const Protocol& p);
// Serialize just one Step (for incremental step requests).
std::string step_to_json(const Step& s, int index, int total);

} // namespace qvis
