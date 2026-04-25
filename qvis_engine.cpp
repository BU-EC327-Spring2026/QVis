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
#include <sstream>
#include <string>

static std::string available_protocol_ids() {
    std::ostringstream out;
    auto protocols = qvis::available_protocols();
    for (size_t i = 0; i < protocols.size(); ++i) {
        if (i) out << ", ";
        out << protocols[i].id;
    }
    return out.str();
}

int main(int argc, char* argv[]) {
    // argv[0] is the program name, argv[1] is the protocol we want
    if (argc < 2) {
        std::cerr << "usage: qvis_engine <" << available_protocol_ids() << ">\n";
        return 1;  // non-zero exit = error (convention across all programs)
    }

    std::string name(argv[1]);

    // build the requested protocol through the shared protocol-object registry.
    // each call returns a fresh Protocol instance with all steps pre-computed.
    qvis::Protocol p;
    try {
        p = qvis::make_protocol(name);
    } catch (const std::invalid_argument&) {
        std::cerr << "unknown protocol: " << name
                  << " (available: " << available_protocol_ids() << ")\n";
        return 1;
    }

    // serialize the Protocol struct to a JSON string and print it.
    // python will capture this output and parse it as a dictionary.
    std::cout << qvis::protocol_to_json(p) << std::flush;
    return 0;
}
