#include <iostream>
#include "quantum_engine.h"

int main() {
    qvis::Protocol p1 = qvis::make_bell_state();
    qvis::Protocol p2 = qvis::make_teleportation();
    qvis::Protocol p3 = qvis::make_grover_2qubit();
    qvis::Protocol p4 = qvis::make_deutsch_jozsa();

    std::cout << "Protocol: " << p1.name << "\n";
    std::cout << "Description: " << p1.description << "\n";
    std::cout << "Qubits: " << p1.n_qubits << "\n";
    std::cout << "Steps: " << p1.steps.size() << "\n\n";

    for (int i = 0; i < (int)p1.steps.size(); i++) {
        qvis::Step& s1 = p1.steps[i];
        std::cout << "Step " << i << ": gate=" << s1.gate_name
                  << "  entropy=" << s1.entropy << "\n";
        std::cout << "  " << s1.narrative << "\n";
    }
    std::cout << "Protocol: " << p2.name << "\n";
    std::cout << "Description: " << p2.description << "\n";
    std::cout << "Qubits: " << p2.n_qubits << "\n";
    std::cout << "Steps: " << p2.steps.size() << "\n\n";

    for (int k = 0; k < (int)p2.steps.size(); k++) {
	    qvis::Step& s2 = p2.steps[k];
	    std::cout << "Step " << k << ": gate=" << s2.gate_name << " entropy=" << s2.entropy << "\n";
	    std::cout << "  " << s2.narrative << "\n";
    }
  //  std::cout << "Protocol: " << p3.name << "\n";
   // std::cout << "Description: " << p3.description << "\n";
  //  std::cout << "Qubits: " << p3.n_qubits << "\n\n";
   // std::cout << "Steps: " << p3.steps.size() << "\n\n";

    	

    return 0;
}
