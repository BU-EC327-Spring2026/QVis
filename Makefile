# QVis — Quantum Protocol Explorer
# EC327 Spring 2026 — Team GAM
#
# Usage:
#   make              → compile qvis_engine (required to use C++ simulation)
#   make test         → compile and run the dev test binary (prints all protocols)
#   make clean        → remove compiled binaries

CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

# main engine binary — called by backend.py as a subprocess
qvis_engine: qvis_engine.cpp quantum_engine.cpp quantum_engine.h
	$(CXX) $(CXXFLAGS) -o qvis_engine qvis_engine.cpp quantum_engine.cpp

# dev test binary — prints all protocols to stdout, useful for debugging
qvis_test: main.cpp quantum_engine.cpp quantum_engine.h
	$(CXX) $(CXXFLAGS) -o qvis_test main.cpp quantum_engine.cpp

test: qvis_test
	./qvis_test

clean:
	rm -f qvis_engine qvis_test

.PHONY: test clean
