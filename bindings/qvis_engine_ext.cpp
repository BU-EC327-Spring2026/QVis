#include <pybind11/pybind11.h>
#include <pybind11/complex.h>
#include <pybind11/stl.h>
#include "state_vector.h"

namespace py = pybind11;

PYBIND11_MODULE(qvis_engine_ext, m) {
    m.doc() = "QVis C++ engine — Python bindings";

    py::class_<qvis::StateVector>(m, "StateVector")
        .def(py::init<std::size_t>(), py::arg("num_qubits"))
        .def("num_qubits", &qvis::StateVector::num_qubits)
        .def("dimension", &qvis::StateVector::dimension)
        .def("__getitem__", [](const qvis::StateVector& sv, std::size_t i) {
            return sv[i];
        })
        .def("__setitem__", [](qvis::StateVector& sv, std::size_t i,
                               std::complex<double> val) {
            sv[i] = val;
        })
        .def("__len__", &qvis::StateVector::dimension);
}
