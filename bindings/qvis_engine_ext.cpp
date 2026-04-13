#include <pybind11/pybind11.h>
#include <pybind11/complex.h>
#include <pybind11/stl.h>
#include "state_vector.h"
#include "gates.h"
#include "measurement.h"

namespace py = pybind11;

PYBIND11_MODULE(qvis_engine_ext, m) {
    m.doc() = "QVis C++ engine — Python bindings";

    py::class_<qvis::StateVector>(m, "StateVector")
        .def(py::init<std::size_t>(), py::arg("num_qubits"))
        .def("num_qubits", &qvis::StateVector::num_qubits)
        .def("dimension", &qvis::StateVector::dimension)
        .def("apply", [](qvis::StateVector& sv,
                         std::array<std::array<std::complex<double>, 2>, 2> mat,
                         std::size_t target_qubit) {
            sv.apply(mat, target_qubit);
        }, py::arg("matrix"), py::arg("target_qubit"))
        .def("apply_controlled", [](qvis::StateVector& sv,
                         std::array<std::array<std::complex<double>, 2>, 2> mat,
                         std::size_t control_qubit, std::size_t target_qubit) {
            sv.apply_controlled(mat, control_qubit, target_qubit);
        }, py::arg("matrix"), py::arg("control_qubit"), py::arg("target_qubit"))
        .def("__getitem__", [](const qvis::StateVector& sv, std::size_t i) {
            return sv[i];
        })
        .def("__setitem__", [](qvis::StateVector& sv, std::size_t i,
                               std::complex<double> val) {
            sv[i] = val;
        })
        .def("__len__", &qvis::StateVector::dimension);

    m.def("hadamard", &qvis::hadamard, "Returns the 2x2 Hadamard gate matrix");
    m.def("pauli_x", &qvis::pauli_x, "Returns the 2x2 Pauli-X (NOT) gate matrix");
    m.def("cnot", &qvis::cnot, py::arg("sv"), py::arg("control"), py::arg("target"),
          "Apply CNOT: flip target qubit when control is |1⟩");
    m.def("sample", &qvis::sample, py::arg("sv"), py::arg("shots"),
          "Sample the state vector, returning bitstring counts");
}
