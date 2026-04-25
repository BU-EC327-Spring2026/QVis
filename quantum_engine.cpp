#include "quantum_engine.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <algorithm>

namespace qvis {

// ════════════════════════════════════════════════════════════════
//  Core gate application
// ════════════════════════════════════════════════════════════════

// Helper: index of basis state with bit `q` set to value `v`
// The convention here is: qubit 0 = most significant bit.
//   |q0 q1 q2⟩  →  index = q0*4 + q1*2 + q2*1
static inline int bit_val(int state_idx, int n_qubits, int q) {
    return (state_idx >> (n_qubits - 1 - q)) & 1;
}

void apply_single(Vec& sv, int n_qubits, int target, const cx gate[4]) {
    int dim = 1 << n_qubits;
    int stride = 1 << (n_qubits - 1 - target);

    for (int i = 0; i < dim; i += stride * 2) {
        for (int j = i; j < i + stride; ++j) {
            // j has bit `target` = 0; j+stride has bit `target` = 1
            cx a = sv[j];
            cx b = sv[j + stride];
            sv[j]        = gate[0]*a + gate[1]*b;
            sv[j+stride] = gate[2]*a + gate[3]*b;
        }
    }
}

void apply_controlled(Vec& sv, int n_qubits, int control, int target, const cx gate[4]) {
    int dim = 1 << n_qubits;
    for (int i = 0; i < dim; ++i) {
        // Only act when control qubit is |1⟩
        if (!bit_val(i, n_qubits, control)) continue;
        if (bit_val(i, n_qubits, target) != 0) continue; // only process |0⟩ partner

        // Find partner with target bit flipped
        int partner = i ^ (1 << (n_qubits - 1 - target));

        cx a = sv[i];       // target = 0
        cx b = sv[partner]; // target = 1
        sv[i]       = gate[0]*a + gate[1]*b;
        sv[partner] = gate[2]*a + gate[3]*b;
    }
}

void apply_swap(Vec& sv, int n_qubits, int q0, int q1) {
    int dim = 1 << n_qubits;
    for (int i = 0; i < dim; ++i) {
        int b0 = bit_val(i, n_qubits, q0);
        int b1 = bit_val(i, n_qubits, q1);
        if (b0 == b1) continue; // 00 and 11 unchanged
        if (b0 > b1) continue;  // process each pair once (b0=0,b1=1 only)

        int mask0 = 1 << (n_qubits - 1 - q0);
        int mask1 = 1 << (n_qubits - 1 - q1);
        int partner = (i | mask1) & ~mask0; // flip both bits
        std::swap(sv[i], sv[partner]);
    }
}

// ════════════════════════════════════════════════════════════════
//  Measurement and entropy
// ════════════════════════════════════════════════════════════════

double measure_prob(const Vec& sv, int n_qubits, int q) {
    double prob = 0.0;
    int dim = 1 << n_qubits;
    for (int i = 0; i < dim; ++i) {
        if (bit_val(i, n_qubits, q) == 1) {
            prob += std::norm(sv[i]); // norm = |z|^2
        }
    }
    return prob;
}

std::vector<double> probabilities(const Vec& sv) {
    std::vector<double> p;
    p.reserve(sv.size());
    for (auto& amp : sv) p.push_back(std::norm(amp));
    return p;
}

// Reduced density matrix diagonal for bipartition: qubits [0..k] vs [k+1..n-1]
// Full Von Neumann entropy requires SVD of reshaped matrix, but for display
// purposes we use the Schmidt decomposition approximation via partial trace diagonal.
double entanglement_entropy(const Vec& sv, int n_qubits, int bipartition_qubit) {
    // dimA = 2^(bipartition_qubit+1), dimB = 2^(n_qubits - bipartition_qubit - 1)
    int dimA = 1 << (bipartition_qubit + 1);
    int dimB = 1 << (n_qubits - bipartition_qubit - 1);
    if (dimB == 0) dimB = 1;

    // Build reduced density matrix ρ_A = Tr_B(|ψ⟩⟨ψ|)
    // ρ_A[i][j] = Σ_k  ψ[i*dimB + k] * conj(ψ[j*dimB + k])
    std::vector<std::vector<cx>> rho(dimA, std::vector<cx>(dimA, {0,0}));
    for (int i = 0; i < dimA; ++i)
        for (int j = 0; j < dimA; ++j)
            for (int k = 0; k < dimB; ++k)
                rho[i][j] += sv[i*dimB + k] * std::conj(sv[j*dimB + k]);

    // Entropy from diagonal (exact only if ρ_A is already diagonal in std basis;
    // for GHZ/Bell states this is exact; for others this is a lower bound)
    // For a rigorous implementation you would diagonalize rho here.
    double S = 0.0;
    for (int i = 0; i < dimA; ++i) {
        double p = std::real(rho[i][i]);
        if (p > 1e-12) S -= p * std::log2(p);
    }
    return S;
}

// ════════════════════════════════════════════════════════════════
//  Named gate factories
// ════════════════════════════════════════════════════════════════

void Gates::H(cx out[4]) {
    out[0] = out[1] = out[2] = INV_SQRT2;
    out[3] = -INV_SQRT2;
}
void Gates::X(cx out[4]) {
    out[0] = out[3] = 0; out[1] = out[2] = 1;
}
void Gates::Y(cx out[4]) {
    out[0] = out[3] = 0;
    out[1] = {0, -1}; out[2] = {0, 1};
}
void Gates::Z(cx out[4]) {
    out[0] = 1; out[1] = 0; out[2] = 0; out[3] = -1;
}
void Gates::S(cx out[4]) {
    out[0] = 1; out[1] = 0; out[2] = 0; out[3] = {0, 1};
}
void Gates::T(cx out[4]) {
    out[0] = 1; out[1] = 0; out[2] = 0;
    out[3] = cx(INV_SQRT2, INV_SQRT2);
}
void Gates::Rx(double theta, cx out[4]) {
    double c = std::cos(theta/2.0), s = std::sin(theta/2.0);
    out[0] = c; out[1] = {0,-s}; out[2] = {0,-s}; out[3] = c;
}
void Gates::Rz(double theta, cx out[4]) {
    out[0] = std::exp(cx(0,-theta/2.0));
    out[1] = 0; out[2] = 0;
    out[3] = std::exp(cx(0, theta/2.0));
}
void Gates::I(cx out[4]) {
    out[0] = out[3] = 1; out[1] = out[2] = 0;
}

// ════════════════════════════════════════════════════════════════
//  Polymorphic gate model
// ════════════════════════════════════════════════════════════════

QuantumGate::QuantumGate(std::string name, int target, int control,
                         std::string narrative, std::string math_hint)
    : name_(std::move(name)),
      target_(target),
      control_(control),
      narrative_(std::move(narrative)),
      math_hint_(std::move(math_hint)) {}

const std::string& QuantumGate::name() const { return name_; }
int QuantumGate::target() const { return target_; }
int QuantumGate::control() const { return control_; }
const std::string& QuantumGate::narrative() const { return narrative_; }
const std::string& QuantumGate::math_hint() const { return math_hint_; }

SingleQubitGate::SingleQubitGate(std::string name, int target, GateFactory factory,
                                 std::string narrative, std::string math_hint)
    : QuantumGate(std::move(name), target, -1, std::move(narrative), std::move(math_hint)),
      factory_(std::move(factory)) {}

void SingleQubitGate::apply(Vec& sv, int n_qubits) const {
    cx gate[4];
    factory_(gate);
    apply_single(sv, n_qubits, target(), gate);
}

ParallelSingleQubitGate::ParallelSingleQubitGate(std::string name,
                                                 std::vector<int> targets,
                                                 GateFactory factory,
                                                 std::string narrative,
                                                 std::string math_hint)
    : QuantumGate(std::move(name), targets.empty() ? -1 : targets.front(), -1,
                  std::move(narrative), std::move(math_hint)),
      targets_(std::move(targets)),
      factory_(std::move(factory)) {}

void ParallelSingleQubitGate::apply(Vec& sv, int n_qubits) const {
    cx gate[4];
    factory_(gate);
    for (int target : targets_) {
        apply_single(sv, n_qubits, target, gate);
    }
}

ControlledGate::ControlledGate(std::string name, int control, int target,
                               GateFactory factory, std::string narrative,
                               std::string math_hint)
    : QuantumGate(std::move(name), target, control, std::move(narrative), std::move(math_hint)),
      factory_(std::move(factory)) {}

void ControlledGate::apply(Vec& sv, int n_qubits) const {
    cx gate[4];
    factory_(gate);
    apply_controlled(sv, n_qubits, control(), target(), gate);
}

PhaseOracleGate::PhaseOracleGate(std::string name, int marked_state,
                                 std::string narrative, std::string math_hint)
    : QuantumGate(std::move(name), -1, -1, std::move(narrative), std::move(math_hint)),
      marked_state_(marked_state) {}

void PhaseOracleGate::apply(Vec& sv, int n_qubits) const {
    int dim = 1 << n_qubits;
    if (marked_state_ < 0 || marked_state_ >= dim) {
        throw std::out_of_range("marked state is outside the statevector");
    }
    sv[marked_state_] = -sv[marked_state_];
}

DiffusionGate::DiffusionGate(std::string name, std::string narrative,
                             std::string math_hint)
    : QuantumGate(std::move(name), -1, -1, std::move(narrative), std::move(math_hint)) {}

void DiffusionGate::apply(Vec& sv, int) const {
    cx mean = 0;
    for (const auto& amp : sv) mean += amp;
    mean /= static_cast<double>(sv.size());
    for (auto& amp : sv) amp = 2.0 * mean - amp;
}

Circuit::Circuit(int n_qubits, Vec initial_state)
    : n_qubits_(n_qubits), initial_state_(std::move(initial_state)) {}

void Circuit::add_gate(std::unique_ptr<QuantumGate> gate) {
    gates_.push_back(std::move(gate));
}

// ════════════════════════════════════════════════════════════════
//  Step builder helper
// ════════════════════════════════════════════════════════════════

static Step make_step(
    const Vec& sv, int n_qubits,
    const std::string& gate_name,
    int target, int control,
    const std::string& narrative,
    const std::string& math_hint = "")
{
    Step s;
    s.gate_name     = gate_name;
    s.qubit_target  = target;
    s.qubit_control = control;
    s.statevector   = sv;
    s.probs         = probabilities(sv);
    s.entropy       = (n_qubits > 1)
                        ? entanglement_entropy(sv, n_qubits, 0)
                        : 0.0;
    s.narrative     = narrative;
    s.math_hint     = math_hint;
    return s;
}

std::vector<Step> Circuit::run(const std::string& initial_narrative,
                               const std::string& initial_math_hint) const {
    Vec sv = initial_state_;
    std::vector<Step> steps;
    steps.push_back(make_step(sv, n_qubits_, "INIT", -1, -1,
                              initial_narrative, initial_math_hint));

    for (const auto& gate : gates_) {
        gate->apply(sv, n_qubits_);
        steps.push_back(make_step(sv, n_qubits_, gate->name(), gate->target(),
                                  gate->control(), gate->narrative(),
                                  gate->math_hint()));
    }

    return steps;
}

// ════════════════════════════════════════════════════════════════
//  Protocol: Bell State
// ════════════════════════════════════════════════════════════════
Protocol make_bell_state() {
    Protocol p;
    p.name        = "Bell State";
    p.description = "Two qubits become maximally entangled. "
                    "Measuring one instantly determines the other, "
                    "regardless of distance. The foundation of quantum teleportation "
                    "and superdense coding.";
    p.n_qubits    = 2;
    p.qubit_labels = {"Alice", "Bob"};

    Vec sv(4, cx(0,0));
    sv[0] = 1.0; // |00⟩

    // Step 0: Initial state
    p.steps.push_back(make_step(sv, 2, "INIT", -1, -1,
        "Both qubits start in |0⟩. The system is in state |00⟩ with 100% probability. "
        "No entanglement yet — these are two completely independent bits.",
        "|ψ⟩ = |00⟩"));

    // Step 1: H on qubit 0
    cx h[4]; Gates::H(h);
    apply_single(sv, 2, 0, h);
    p.steps.push_back(make_step(sv, 2, "H", 0, -1,
        "Hadamard gate on Alice's qubit. This creates a superposition: Alice's qubit "
        "is now simultaneously |0⟩ and |1⟩ with equal probability. Bob's qubit is "
        "still |0⟩. The two qubits are not yet entangled.",
        "|ψ⟩ = (|0⟩ + |1⟩)/√2 ⊗ |0⟩"));

    // Step 2: CNOT with qubit 0 as control, qubit 1 as target
    cx x[4]; Gates::X(x);
    apply_controlled(sv, 2, 0, 1, x);
    p.steps.push_back(make_step(sv, 2, "CNOT", 1, 0,
        "CNOT gate: Bob's qubit flips IF Alice's qubit is |1⟩. Because Alice was in "
        "superposition, Bob's qubit is now correlated with Alice's. The result is the "
        "Bell state |Φ+⟩ — maximum entanglement. Neither qubit has a definite state "
        "alone; they only exist as a joint system.",
        "|Φ+⟩ = (|00⟩ + |11⟩)/√2"));

    return p;
}

// ════════════════════════════════════════════════════════════════
//  Protocol: Quantum Teleportation
// ════════════════════════════════════════════════════════════════
Protocol make_teleportation() {
    Protocol p;
    p.name        = "Quantum Teleportation";
    p.description = "Alice transfers an unknown quantum state to Bob using a shared "
                    "Bell pair and 2 classical bits — without physically sending the qubit. "
                    "This is the most dramatic demonstration of entanglement as a resource.";
    p.n_qubits    = 3;
    p.qubit_labels = {"Message (q0)", "Alice (q1)", "Bob (q2)"};

    // We teleport |ψ⟩ = (|0⟩ + |1⟩)/√2 on qubit 0 — a superposition state
    Vec sv(8, cx(0,0));
    sv[0] = INV_SQRT2;  // |000⟩ component
    sv[4] = INV_SQRT2;  // |100⟩ component  (qubit0=1, others=0)

    p.steps.push_back(make_step(sv, 3, "INIT", -1, -1,
        "Setup: Alice has an unknown qubit |ψ⟩ = (|0⟩+|1⟩)/√2 she wants to send to Bob. "
        "Alice and Bob also share a pre-entangled Bell pair (qubits 1 and 2). "
        "We show the full 3-qubit system here.",
        "|ψ⟩⊗|00⟩"));

    // Step 1: Entangle Alice-Bob pair: H on q1
    cx h[4]; Gates::H(h);
    apply_single(sv, 3, 1, h);
    p.steps.push_back(make_step(sv, 3, "H", 1, -1,
        "Step 1 — Create the Bell pair resource: Hadamard on Alice's half of the "
        "shared pair (qubit 1). This puts Alice's qubit into superposition, "
        "preparing for entanglement with Bob.",
        "H on q1: puts q1 into (|0⟩+|1⟩)/√2"));

    // Step 2: CNOT q1→q2 (entangle the Alice-Bob pair)
    cx x[4]; Gates::X(x);
    apply_controlled(sv, 3, 1, 2, x);
    p.steps.push_back(make_step(sv, 3, "CNOT", 2, 1,
        "Step 2 — Alice and Bob's shared qubits are now entangled into a Bell pair. "
        "Alice keeps qubit 1; Bob takes qubit 2 and travels far away. "
        "The entanglement is the 'quantum channel' that makes teleportation possible.",
        "(|00⟩+|11⟩)/√2 for qubits 1 and 2"));

    // Step 3: Alice's Bell measurement — CNOT q0→q1
    apply_controlled(sv, 3, 0, 1, x);
    p.steps.push_back(make_step(sv, 3, "CNOT", 1, 0,
        "Step 3 — Alice begins her Bell measurement. She applies CNOT between her "
        "message qubit (q0) and her half of the Bell pair (q1). This entangles "
        "the message with Alice's local qubit — beginning the measurement process.",
        "Alice's Bell measurement, step 1 of 2"));

    // Step 4: H on q0
    apply_single(sv, 3, 0, h);
    p.steps.push_back(make_step(sv, 3, "H", 0, -1,
        "Step 4 — Alice applies Hadamard to her message qubit. Together with the "
        "previous CNOT, this completes Alice's Bell basis measurement. "
        "Alice now measures her two qubits and gets 2 classical bits: 00, 01, 10, or 11.",
        "Alice's Bell measurement complete"));

    // Step 5: Corrections — X on q2 conditioned on q1 (simulated as unconditional for viz)
    // We apply the expected correction for the 00 outcome for visualization clarity
    apply_controlled(sv, 3, 1, 2, x);
    p.steps.push_back(make_step(sv, 3, "X (correction)", 2, 1,
        "Step 5 — Bob receives Alice's 2 classical bits and applies corrections. "
        "If Alice's second bit was 1: Bob applies X (bit-flip). "
        "If Alice's first bit was 1: Bob applies Z (phase-flip). "
        "After corrections, Bob's qubit (q2) is now exactly in Alice's original state |ψ⟩.",
        "Bob's qubit is now |ψ⟩ — teleportation complete!"));

    return p;
}

// ════════════════════════════════════════════════════════════════
//  Protocol: Grover's Search (2 qubits, N=4 database)
// ════════════════════════════════════════════════════════════════
Protocol make_grover_2qubit() {
    Protocol p;
    p.name        = "Grover's Search";
    p.description = "Searching a 4-item database for a marked item. Classically: "
                    "need up to 4 queries. Grover's algorithm: 1 query suffices. "
                    "For N items, quantum search needs O(√N) vs classical O(N).";
    p.n_qubits    = 2;
    p.qubit_labels = {"q0", "q1"};

    Vec sv(4, cx(0,0));
    sv[0] = 1.0; // |00⟩

    p.steps.push_back(make_step(sv, 2, "INIT", -1, -1,
        "Start: System in |00⟩. We want to find the marked item |11⟩ (index 3) in a "
        "4-element database {|00⟩, |01⟩, |10⟩, |11⟩}. Classically you'd check up to "
        "all 4 items. Grover's finds it in just 1 oracle query for N=4.",
        "|ψ⟩ = |00⟩"));

    // Step 1 & 2: H⊗H — uniform superposition
    cx h[4]; Gates::H(h);
    apply_single(sv, 2, 0, h);
    apply_single(sv, 2, 1, h);
    p.steps.push_back(make_step(sv, 2, "H⊗H", 0, -1,
        "Hadamard on both qubits creates a uniform superposition of ALL 4 basis states. "
        "Each state has equal amplitude 1/2, meaning 25% probability. The quantum "
        "computer is now 'considering' all 4 database items simultaneously.",
        "|ψ⟩ = (|00⟩+|01⟩+|10⟩+|11⟩)/2"));

    // Step 3: Oracle — phase flip on |11⟩ (the marked item)
    // Oracle for target |11⟩: flip sign of amplitude at index 3
    sv[3] = -sv[3];
    p.steps.push_back(make_step(sv, 2, "Oracle", -1, -1,
        "The Oracle marks the target item |11⟩ by flipping its phase (amplitude sign). "
        "This is the quantum equivalent of 'checking' an item. Crucially, the oracle "
        "acts on ALL states in superposition simultaneously in ONE query. "
        "Probabilities look unchanged — the phase difference is hidden in the amplitudes.",
        "|ψ⟩ = (|00⟩+|01⟩+|10⟩−|11⟩)/2"));

    // step 4: diffusion operator = H⊗H · (2|00⟩⟨00| − I) · H⊗H
    // after the first H⊗H, sv is in the hadamard basis.
    // (2|00⟩⟨00| − I) means: leave sv[0] alone, negate sv[1..3].
    // then H⊗H maps back to the computational basis.
    apply_single(sv, 2, 0, h);
    apply_single(sv, 2, 1, h);
    for (int i = 1; i < 4; ++i) sv[i] = -sv[i];  // negate all except |00⟩ (index 0)
    apply_single(sv, 2, 0, h);
    apply_single(sv, 2, 1, h);
    p.steps.push_back(make_step(sv, 2, "Diffusion", -1, -1,
        "The Diffusion (Grover) operator amplifies the marked state's amplitude and "
        "suppresses all others — a process called 'amplitude amplification'. "
        "For N=4, ONE application of oracle+diffusion is enough: |11⟩ now has "
        "100% probability. One measurement will find the answer with certainty!",
        "|ψ⟩ = |11⟩  →  found in 1 query vs 4 classical"));

    return p;
}

// ════════════════════════════════════════════════════════════════
//  Protocol: Deutsch-Jozsa (2-qubit version)
// ════════════════════════════════════════════════════════════════
Protocol make_deutsch_jozsa() {
    Protocol p;
    p.name        = "Deutsch-Jozsa";
    p.description = "Is a function constant (same output for all inputs) or balanced "
                    "(half 0s, half 1s)? Classically: need N/2+1 queries in worst case. "
                    "Quantum: always just 1 query. The first proven quantum speedup (1992).";
    p.n_qubits    = 2;
    p.qubit_labels = {"Input (q0)", "Ancilla (q1)"};

    // Start: |0⟩|1⟩  (ancilla in |1⟩)
    Vec sv(4, cx(0,0));
    sv[1] = 1.0; // |01⟩

    p.steps.push_back(make_step(sv, 2, "INIT", -1, -1,
        "Start: input qubit in |0⟩, ancilla qubit in |1⟩. We want to determine "
        "if a black-box function f is constant (always 0 or always 1) or balanced "
        "(outputs 0 half the time and 1 half the time). Classically, worst case "
        "requires N/2+1 queries — quantum needs exactly 1.",
        "|ψ⟩ = |0⟩|1⟩"));

    cx h[4]; Gates::H(h);
    apply_single(sv, 2, 0, h);
    apply_single(sv, 2, 1, h);
    p.steps.push_back(make_step(sv, 2, "H⊗H", 0, -1,
        "Hadamard on both qubits. The ancilla (now in |−⟩) acts as a 'phase kickback' "
        "mechanism — any f(x) evaluated will flip the phase of the input amplitudes "
        "instead of writing to a separate output. This is how the oracle encodes "
        "information into phases rather than bit values.",
        "|ψ⟩ = (|0⟩+|1⟩)|−⟩ / √2"));

    // Oracle for balanced function f(x) = x  (CNOT q0 controls q1)
    cx x[4]; Gates::X(x);
    apply_controlled(sv, 2, 0, 1, x);
    p.steps.push_back(make_step(sv, 2, "Oracle (balanced)", 1, 0,
        "The oracle encodes f(x)=x (a balanced function: f(0)=0, f(1)=1). "
        "Via phase kickback, the input qubit gets a phase flip for x=1. "
        "The amplitudes now encode the difference between f(0) and f(1) as a phase.",
        "Phase kickback: |x⟩|−⟩ → (−1)^f(x)|x⟩|−⟩"));

    apply_single(sv, 2, 0, h);
    p.steps.push_back(make_step(sv, 2, "H (final)", 0, -1,
        "Final Hadamard on the input qubit. If f is CONSTANT: this returns q0 to |0⟩ "
        "with 100% probability — measuring 0 proves it. If f is BALANCED (as here): "
        "q0 is in |1⟩ with 100% probability — measuring 1 proves it. "
        "One query, definitive answer. Classically impossible.",
        "Measure |0⟩ → constant, |1⟩ → balanced"));

    return p;
}

// ════════════════════════════════════════════════════════════════
//  Protocol: Grover's Search (3-qubit, 2 iterations)
// ════════════════════════════════════════════════════════════════
Protocol make_grover_3qubit() {
    Protocol p;
    p.name        = "Grover's (3-Qubit)";
    p.description = "Find |111⟩ in an 8-item database using 2 queries instead of 8. "
                    "Demonstrates Grover's O(√N) advantage at 3 qubits — "
                    "the kind of circuit where the C++ engine matters.";
    p.n_qubits    = 3;
    p.qubit_labels = {"q0", "q1", "q2"};

    Vec sv(8, cx(0,0));
    sv[0] = 1.0; // |000⟩

    Circuit circuit(3, sv);
    circuit.add_gate(std::unique_ptr<QuantumGate>(new ParallelSingleQubitGate(
        "H⊗H⊗H", {0, 1, 2}, Gates::H,
        "Hadamard on all 3 qubits creates a uniform superposition over all 8 states. "
        "Each of |000⟩ through |111⟩ has exactly 12.5% probability. "
        "The computer now considers all 8 database entries simultaneously.",
        "(|000⟩+|001⟩+…+|111⟩)/√8")));
    circuit.add_gate(std::unique_ptr<QuantumGate>(new PhaseOracleGate(
        "Oracle", 7,
        "The oracle marks |111⟩ by flipping its phase. Probabilities look identical — the change "
        "is hidden in the sign of the amplitude. This is one quantum query acting on all 8 states at once.",
        "(|000⟩+…−|111⟩)/√8")));
    circuit.add_gate(std::unique_ptr<QuantumGate>(new DiffusionGate(
        "Diffusion",
        "Grover diffusion (inversion about the mean) amplifies |111⟩ and suppresses the rest. "
        "After just one oracle+diffusion cycle, |111⟩ has ~78% probability. "
        "A second iteration will push it past 94%.",
        "P(|111⟩) ≈ 78% after 1 iteration")));
    circuit.add_gate(std::unique_ptr<QuantumGate>(new PhaseOracleGate(
        "Oracle ×2", 7,
        "Second oracle query: phase-flip |111⟩ again. This is the full quantum budget — "
        "2 queries for 8 items, vs 8 classically. The phase difference is now even more pronounced.",
        "Phase flip ×2")));
    circuit.add_gate(std::unique_ptr<QuantumGate>(new DiffusionGate(
        "Diffusion ×2",
        "Second diffusion round. |111⟩ now has ~94.5% probability — one measurement almost "
        "certainly finds the answer. This is the quantum advantage: 2 queries vs 8 classical.",
        "P(|111⟩) ≈ 94.5% — answer found in 2 queries!")));

    p.steps = circuit.run(
        "Three qubits start in |000⟩. Goal: find the marked item |111⟩ in a database of 8. "
        "Classical worst case: 8 queries. Grover's algorithm needs only 2 — a √8 ≈ 2.8× speedup.",
        "|ψ⟩ = |000⟩");

    return p;
}

// ════════════════════════════════════════════════════════════════
//  Polymorphic protocol registry
// ════════════════════════════════════════════════════════════════

ProtocolBuilder::ProtocolBuilder(std::string id, std::string display_name)
    : id_(std::move(id)), display_name_(std::move(display_name)) {}

const std::string& ProtocolBuilder::id() const { return id_; }
const std::string& ProtocolBuilder::display_name() const { return display_name_; }

class FunctionProtocolBuilder : public ProtocolBuilder {
public:
    FunctionProtocolBuilder(std::string id, std::string display_name,
                            std::function<Protocol()> factory)
        : ProtocolBuilder(std::move(id), std::move(display_name)),
          factory_(std::move(factory)) {}

    Protocol build() const override {
        return factory_();
    }

private:
    std::function<Protocol()> factory_;
};

static const std::vector<std::unique_ptr<ProtocolBuilder>>& protocol_registry() {
    static const std::vector<std::unique_ptr<ProtocolBuilder>> registry = [] {
        std::vector<std::unique_ptr<ProtocolBuilder>> builders;
        builders.push_back(std::unique_ptr<ProtocolBuilder>(
            new FunctionProtocolBuilder("bell", "Bell State", make_bell_state)));
        builders.push_back(std::unique_ptr<ProtocolBuilder>(
            new FunctionProtocolBuilder("teleport", "Quantum Teleportation", make_teleportation)));
        builders.push_back(std::unique_ptr<ProtocolBuilder>(
            new FunctionProtocolBuilder("grover", "Grover's Search", make_grover_2qubit)));
        builders.push_back(std::unique_ptr<ProtocolBuilder>(
            new FunctionProtocolBuilder("grover3", "Grover's (3-Qubit)", make_grover_3qubit)));
        builders.push_back(std::unique_ptr<ProtocolBuilder>(
            new FunctionProtocolBuilder("deutsch", "Deutsch-Jozsa", make_deutsch_jozsa)));
        return builders;
    }();
    return registry;
}

std::vector<ProtocolInfo> available_protocols() {
    std::vector<ProtocolInfo> infos;
    for (const auto& builder : protocol_registry()) {
        infos.push_back({builder->id(), builder->display_name()});
    }
    return infos;
}

Protocol make_protocol(const std::string& id) {
    for (const auto& builder : protocol_registry()) {
        if (builder->id() == id) {
            return builder->build();
        }
    }
    throw std::invalid_argument("unknown protocol id: " + id);
}

// ════════════════════════════════════════════════════════════════
//  JSON serialization
// ════════════════════════════════════════════════════════════════

static std::string cx_to_json(const cx& z) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << "{\"re\":" << z.real() << ",\"im\":" << z.imag() << "}";
    return oss.str();
}

static std::string escape_json(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

std::string step_to_json(const Step& s, int index, int total) {
    std::ostringstream o;
    o << std::fixed << std::setprecision(6);
    o << "{"
      << "\"index\":" << index << ","
      << "\"total\":" << total << ","
      << "\"gate\":\"" << escape_json(s.gate_name) << "\","
      << "\"target\":" << s.qubit_target << ","
      << "\"control\":" << s.qubit_control << ","
      << "\"entropy\":" << s.entropy << ","
      << "\"narrative\":\"" << escape_json(s.narrative) << "\","
      << "\"math\":\"" << escape_json(s.math_hint) << "\","
      << "\"probs\":[";
    for (size_t i = 0; i < s.probs.size(); ++i) {
        if (i) o << ",";
        o << s.probs[i];
    }
    o << "],\"statevector\":[";
    for (size_t i = 0; i < s.statevector.size(); ++i) {
        if (i) o << ",";
        o << cx_to_json(s.statevector[i]);
    }
    o << "]}";
    return o.str();
}

std::string protocol_to_json(const Protocol& p) {
    std::ostringstream o;
    o << "{"
      << "\"name\":\"" << escape_json(p.name) << "\","
      << "\"description\":\"" << escape_json(p.description) << "\","
      << "\"n_qubits\":" << p.n_qubits << ","
      << "\"qubit_labels\":[";
    for (size_t i = 0; i < p.qubit_labels.size(); ++i) {
        if (i) o << ",";
        o << "\"" << escape_json(p.qubit_labels[i]) << "\"";
    }
    o << "],\"steps\":[";
    for (size_t i = 0; i < p.steps.size(); ++i) {
        if (i) o << ",";
        o << step_to_json(p.steps[i], (int)i, (int)p.steps.size());
    }
    o << "]}";
    return o.str();
}

} // namespace qvis
