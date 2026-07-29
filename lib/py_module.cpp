#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <simulator.h>
#include <circuit.h>

namespace py = pybind11;

PYBIND11_MODULE(pyvacask, m, py::mod_gil_not_used()) {
	m.doc() = "VACASK Simulator.";
	m.def("startupPath", &sim::Simulator::startupPath, "Simulator start-up path.");
	
	// pyvacask.status
	{
		auto mod_status = m.def_submodule("status");
		py::class_<sim::Status>(mod_status, "Status")
			.def(py::init())
			.def("message", &sim::Status::message);
	}

	// pyvacask.simulator
	{
		auto sim_status = m.def_submodule("simulator");
		sim_status.def(
		    "setup",
		    [](py::object obj) {
		        if (obj.is_none()) {
		            return sim::Simulator::setup();
		        }
		        return sim::Simulator::setup(obj.cast<sim::Status&>());
		    },
		    py::arg("s") = py::none());
		sim_status.def(
		    "prependModulePath",
		    [](std::vector<std::string> paths) {
		        sim::Simulator::prependModulePath(std::move(paths));
		    },
		    py::arg("paths")
		);
		sim_status.def(
		    "appendModulePath",
		    [](std::vector<std::string> paths) {
		        sim::Simulator::appendModulePath(std::move(paths));
		    },
		    py::arg("paths")
		);
		sim_status.def(
		    "prependIncludePath",
		    [](std::vector<std::string> paths) {
		        sim::Simulator::prependIncludePath(std::move(paths));
		    },
		    py::arg("paths")
		);
		sim_status.def(
		    "appendIncludePath",
		    [](std::vector<std::string> paths) {
		        sim::Simulator::appendIncludePath(std::move(paths));
		    },
		    py::arg("paths")
		);
	}
	
	// pyvacask.parser_output
	{
		auto mod_parser_output = m.def_submodule("parser_output");
		py::class_<sim::ParserTables>(mod_parser_output, "ParserTables")
			.def(py::init<const std::string&>())
                        .def_property_readonly("title", &sim::ParserTables::title)
                        .def_property_readonly("fileStack", &sim::ParserTables::fileStack)
                        .def_property_readonly("loads", &sim::ParserTables::loads)
                        .def_property_readonly("groundNodes", &sim::ParserTables::groundNodes)
                        .def_property_readonly("globalNodes", &sim::ParserTables::globalNodes)
                        .def_property_readonly("embed", &sim::ParserTables::embed)
                        .def_property_readonly("accounting", &sim::ParserTables::accounting)
			.def("setTitle",
                            [](sim::ParserTables& self, const std::string& t) -> sim::ParserTables& {
                                return self.setTitle(t);
                            },
                            py::return_value_policy::reference_internal)
                        .def("setDefaultSubDef",
                             [](sim::ParserTables& self, sim::PTSubcircuitDefinition& def) -> sim::ParserTables& {
                                 return self.setDefaultSubDef(std::move(def));
                             },
                             py::return_value_policy::reference_internal)
                        .def("add",
                             [](sim::ParserTables& self, sim::PTLoad& ld) -> sim::ParserTables& {
                                 return self.add(std::move(ld));
                             },
                             py::return_value_policy::reference_internal)
                        .def("addGround",
                             [](sim::ParserTables& self, sim::PTParsedIdentifier& parsedId) -> sim::ParserTables& {
                                 return self.addGround(std::move(parsedId));
                             },
                             py::return_value_policy::reference_internal)
                        .def("addGlobal",
                            [](sim::ParserTables& self, sim::PTParsedIdentifier& parsedId) -> sim::ParserTables& {
                                return self.addGlobal(std::move(parsedId));
                            },
                            py::return_value_policy::reference_internal)
                        .def("defaultGround",
                             [](sim::ParserTables& self) -> sim::ParserTables& { return self.defaultGround(); },
                             py::return_value_policy::reference_internal
                        );
	}
}
