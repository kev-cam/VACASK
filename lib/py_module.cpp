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

                py::class_<sim::PTModel>(mod_parser_output, "PTModel")
                        .def(py::init<>())
                        .def(py::init<sim::Id, sim::Id, const sim::Loc&>(),
                             py::arg("name"),
                             py::arg("device"),
                             py::arg("location") = sim::Loc::bad)
                        .def(py::init([](sim::Id name,
                                         sim::Id device,
                                         sim::PTParameters& params,
                                         const sim::Loc& loc) {
                                return sim::PTModel(
                                    name,
                                    device,
                                    std::move(params),
                                    loc);
                             }),
                             py::arg("name"),
                             py::arg("device"),
                             py::arg("parameters"),
                             py::arg("location") = sim::Loc::bad)
                        .def("location",
                             &sim::PTModel::location,
                             py::return_value_policy::reference_internal)
                        .def("name", &sim::PTModel::name)
                        .def("device", &sim::PTModel::device)
                        .def("isParameterized", &sim::PTModel::isParameterized)
                        .def("parameters",
                             &sim::PTModel::parameters,
                             py::return_value_policy::reference_internal)
                        // Fluent API:
                        .def("add",
                             [](sim::PTModel& self, sim::PTParameters& par) -> sim::PTModel& {
                                 return self.add(std::move(par));
                             },
                             py::return_value_policy::reference_internal)
                        .def("add",
                             [](sim::PTModel& self, sim::PTParameterValue& value) -> sim::PTModel& {
                                 return self.add(std::move(value));
                             },
                             py::return_value_policy::reference_internal)
                        .def("add",
                             [](sim::PTModel& self, sim::PTParameterExpression& expr) -> sim::PTModel& {
                                 return self.add(std::move(expr));
                             },
                             py::return_value_policy::reference_internal)
                        .def("dump",
                             [](const sim::PTModel& self, int indent) {
                                 self.dump(indent, std::cout);
                             },
                             py::arg("indent"))
                        .def("verify",
                             [](const sim::PTModel& self, int level) {
                                 sim::Status status;
                                 return self.verify(level, status);
                             },
                             py::arg("level"));

                py::class_<sim::PTInstance>(mod_parser_output, "PTInstance")
                        // Constructors
                        .def(py::init<>())
                        .def(py::init([](sim::Id name,
                                         sim::Id master,
                                         sim::PTIdentifierList& terms,
                                         const sim::Loc& loc) {
                                return sim::PTInstance(
                                    name,
                                    master,
                                    std::move(terms),
                                    loc);
                             }),
                             py::arg("name"),
                             py::arg("master"),
                             py::arg("terms"),
                             py::arg("location") = sim::Loc::bad)
                        .def(py::init([](sim::Id name,
                                         sim::Id master,
                                         sim::PTIdentifierList& terms,
                                         sim::PTParameters& params,
                                         const sim::Loc& loc) {
                                return sim::PTInstance(
                                    name,
                                    master,
                                    std::move(terms),
                                    std::move(params),
                                    loc);
                             }),
                             py::arg("name"),
                             py::arg("master"),
                             py::arg("terms"),
                             py::arg("parameters"),
                             py::arg("location") = sim::Loc::bad)
                        // Getters
                        .def("location",
                             &sim::PTInstance::location,
                             py::return_value_policy::reference_internal)
                        .def("name", &sim::PTInstance::name)
                        .def("masterName", &sim::PTInstance::masterName)
                        .def("isParameterized", &sim::PTInstance::isParameterized)
                        .def("parameters",
                             &sim::PTInstance::parameters,
                             py::return_value_policy::reference_internal)
                        .def("connections",
                             &sim::PTInstance::connections,
                             py::return_value_policy::reference_internal)
                        // Fluent API
                        .def("add",
                             [](sim::PTInstance& self, sim::PTParameters& par)
                                 -> sim::PTInstance& {
                                 return self.add(std::move(par));
                             },
                             py::return_value_policy::reference_internal)
                        .def("add",
                             [](sim::PTInstance& self, sim::PTParameterValue& value)
                                 -> sim::PTInstance& {
                                 return self.add(std::move(value));
                             },
                             py::return_value_policy::reference_internal)
                        .def("add",
                             [](sim::PTInstance& self, sim::PTParameterExpression& expr)
                                 -> sim::PTInstance& {
                                 return self.add(std::move(expr));
                             },
                             py::return_value_policy::reference_internal)
                        // Utilities
                        .def("dump",
                             [](const sim::PTInstance& self, int indent) {
                                 self.dump(indent, std::cout);
                             },
                             py::arg("indent"))
                        .def("verify",
                             [](const sim::PTInstance& self, int level) {
                                 sim::Status status;
                                 return self.verify(level, status);
                             },
                             py::arg("level"));
	}
}
