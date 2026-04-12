#include <pybind11/pybind11.h>
#include <pybind11/complex.h>
#include <pybind11/stl.h>
#include "state_vector.h"
#include "gates.h"

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
        .def("__getitem__", [](const qvis::StateVector& sv, std::size_t i) {
            return sv[i];
        })
        .def("__setitem__", [](qvis::StateVector& sv, std::size_t i,
                               std::complex<double> val) {
            sv[i] = val;
        })
        .def("__len__", &qvis::StateVector::dimension);

    m.def("hadamard", &qvis::hadamard, "Returns the 2x2 Hadamard gate matrix");
}
