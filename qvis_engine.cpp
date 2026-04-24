// qvis_engine.cpp — command-line entry point for the C++ simulation engine.
//
// usage: ./qvis_engine <protocol_name>
//   prints the full protocol as JSON to stdout, then exits.
//   python's backend.py calls this as a subprocess and reads that output.
//
// the actual simulation math lives in quantum_engine.cpp.
// this file just wires the command-line argument to the right protocol function.

#include "quantum_engine.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // argv[0] is the program name, argv[1] is the protocol we want
    if (argc < 2) {
        std::cerr << "usage: qvis_engine <bell|teleport|grover|deutsch>\n";
        return 1;  // non-zero exit = error (convention across all programs)
    }

    std::string name(argv[1]);

    // build the requested protocol — each function runs the full simulation
    // and returns a Protocol struct with all steps pre-computed
    qvis::Protocol p;
    if      (name == "bell")     p = qvis::make_bell_state();
    else if (name == "teleport") p = qvis::make_teleportation();
    else if (name == "grover")   p = qvis::make_grover_2qubit();
    else if (name == "grover3")  p = qvis::make_grover_3qubit();
    else if (name == "deutsch")  p = qvis::make_deutsch_jozsa();
    else {
        std::cerr << "unknown protocol: " << name
                  << " (available: bell, teleport, grover, grover3, deutsch)\n";
        return 1;
    }

    // serialize the Protocol struct to a JSON string and print it.
    // python will capture this output and parse it as a dictionary.
    std::cout << qvis::protocol_to_json(p) << std::flush;
    return 0;
}
