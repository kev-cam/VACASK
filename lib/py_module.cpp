#include <pybind11/pybind11.h>
#include <simulator.h>

namespace py = pybind11;

PYBIND11_MODULE(pyvacask, m, py::mod_gil_not_used()) {
	m.doc() = "VACASK Simulator.";
	m.def("startupPath", &sim::Simulator::startupPath, "Simulator start-up path.");
}
