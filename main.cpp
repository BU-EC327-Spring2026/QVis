#include <iostream>
#include "quantum_engine.h"

int main() {
    for (const auto& info : qvis::available_protocols()) {
        qvis::Protocol protocol = qvis::make_protocol(info.id);

        std::cout << "Protocol: " << protocol.name << "\n";
        std::cout << "ID: " << info.id << "\n";
        std::cout << "Description: " << protocol.description << "\n";
        std::cout << "Qubits: " << protocol.n_qubits << "\n";
        std::cout << "Steps: " << protocol.steps.size() << "\n\n";

        for (int i = 0; i < (int)protocol.steps.size(); i++) {
            qvis::Step& step = protocol.steps[i];
            std::cout << "Step " << i << ": gate=" << step.gate_name
                      << " entropy=" << step.entropy << "\n";
            std::cout << "  " << step.narrative << "\n";
        }
        std::cout << "\n";
    }

    return 0;
}
