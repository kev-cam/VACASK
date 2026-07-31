#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <simulator.h>
#include <circuit.h>
#include "openvafcomp.h"

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

                py::class_<sim::PTSubcircuitDefinition, sim::PTModel>(
                        mod_parser_output, "PTSubcircuitDefinition")
                        // Constructors
                        .def(py::init<>())
                        .def(py::init<sim::Id, const sim::Loc&>(),
                             py::arg("name"),
                             py::arg("location") = sim::Loc::bad)
                        .def(py::init([](sim::Id name,
                                         sim::PTIdentifierList& terms,
                                         const sim::Loc& loc) {
                                return sim::PTSubcircuitDefinition(
                                    name,
                                    std::move(terms),
                                    loc);
                             }),
                             py::arg("name"),
                             py::arg("terminals"),
                             py::arg("location") = sim::Loc::bad)
                        // Getters
                        .def("terminals",
                             &sim::PTSubcircuitDefinition::terminals,
                             py::return_value_policy::reference_internal)
                        .def("root",
                             &sim::PTSubcircuitDefinition::root,
                             py::return_value_policy::reference_internal)
                        // subDefs accessors
                        .def("subDefCount",
                             [](const sim::PTSubcircuitDefinition& self) {
                                 return self.subDefs().size();
                             })
                        .def("subDef",
                             [](sim::PTSubcircuitDefinition& self, size_t i)
                                 -> sim::PTSubcircuitDefinition& {
                                 return *self.subDefs().at(i);
                             },
                             py::return_value_policy::reference_internal)
                        .def("add",
                             [](sim::PTSubcircuitDefinition& self,
                                sim::PTSubcircuitDefinition& sub)
                                 -> sim::PTSubcircuitDefinition& {
                                 return self.add(std::move(sub));
                             },
                             py::return_value_policy::reference_internal)
                        .def("add",
                             [](sim::PTSubcircuitDefinition& self,
                                sim::PTModel& model)
                                 -> sim::PTSubcircuitDefinition& {
                                 return self.add(std::move(model));
                             },
                             py::return_value_policy::reference_internal)
                        .def("add",
                             [](sim::PTSubcircuitDefinition& self,
                                sim::PTInstance& inst)
                                 -> sim::PTSubcircuitDefinition& {
                                 return self.add(std::move(inst));
                             },
                             py::return_value_policy::reference_internal)
                        .def("add",
                             [](sim::PTSubcircuitDefinition& self,
                                sim::PTBlockSequence& seq)
                                 -> sim::PTSubcircuitDefinition& {
                                 return self.add(std::move(seq));
                             },
                             py::return_value_policy::reference_internal)
                        .def("add",
                             [](sim::PTSubcircuitDefinition& self,
                                sim::PTParameters& params)
                                 -> sim::PTSubcircuitDefinition& {
                                 return self.add(std::move(params));
                             },
                             py::return_value_policy::reference_internal)
                        .def("add",
                             [](sim::PTSubcircuitDefinition& self,
                                sim::PTParameterValue& value)
                                 -> sim::PTSubcircuitDefinition& {
                                 return self.add(std::move(value));
                             },
                             py::return_value_policy::reference_internal)
                        .def("add",
                             [](sim::PTSubcircuitDefinition& self,
                                sim::PTParameterExpression& expr)
                                 -> sim::PTSubcircuitDefinition& {
                                 return self.add(std::move(expr));
                             },
                             py::return_value_policy::reference_internal)
                        .def("dump",
                             [](const sim::PTSubcircuitDefinition& self, int indent) {
                                 self.dump(indent, std::cout);
                             },
                             py::arg("indent"))
                        .def("verify",
                             [](const sim::PTSubcircuitDefinition& self, int level) {
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

                py::class_<sim::PTParameters>(mod_parser_output, "PTParameters")
                        // Constructors
                        .def(py::init<>())
                        // Getters
                        .def("valueCount", &sim::PTParameters::valueCount)
                        .def("expressionCount", &sim::PTParameters::expressionCount)
                        .def("count", &sim::PTParameters::count)
                        .def("values",
                             static_cast<const std::vector<sim::PTParameterValue>&
                                 (sim::PTParameters::*)() const>(&sim::PTParameters::values),
                             py::return_value_policy::reference_internal)
                        .def("expressions",
                             static_cast<const std::vector<sim::PTParameterExpression>&
                                 (sim::PTParameters::*)() const>(&sim::PTParameters::expressions),
                             py::return_value_policy::reference_internal)
                        // Fluent API
                        .def("add",
                             [](sim::PTParameters& self,
                                sim::PTParameterValue& value) -> sim::PTParameters& {
                                 return self.add(std::move(value));
                             },
                             py::return_value_policy::reference_internal)
                        .def("add",
                             [](sim::PTParameters& self,
                                sim::PTParameterExpression& expr) -> sim::PTParameters& {
                                 return self.add(std::move(expr));
                             },
                             py::return_value_policy::reference_internal)
                        .def("add",
                             [](sim::PTParameters& self,
                                sim::PTParameters& params) -> sim::PTParameters& {
                                 return self.add(std::move(params));
                             },
                             py::return_value_policy::reference_internal)
                        // Utilities
                        .def("verify",
                             [](const sim::PTParameters& self, int level) {
                                 sim::Status status;
                                 return self.verify(level, status);
                             },
                             py::arg("level"))
                        .def("__repr__",
                             [](const sim::PTParameters& self) {
                                 std::ostringstream os;
                                 os << self;
                                 return os.str();
                             });

                py::class_<sim::PTParameterValue>(mod_parser_output, "PTParameterValue")
                        .def(py::init([](sim::Id name,
                                         sim::Value& value,
                                         const sim::Loc& loc) {
                                return sim::PTParameterValue(
                                    name,
                                    std::move(value),
                                    loc);
                             }),
                             py::arg("name"),
                             py::arg("value"),
                             py::arg("location") = sim::Loc::bad)
                        .def("name", &sim::PTParameterValue::name)
                        .def("location", &sim::PTParameterValue::location)
                        .def("val",
                             &sim::PTParameterValue::val,
                             py::return_value_policy::reference_internal)
                        .def("dump",
                             [](const sim::PTParameterValue& self, int indent) {
                                 self.dump(indent, std::cout);
                             });

                py::class_<sim::PTParameterExpression>(mod_parser_output, "PTParameterExpression")
                        .def(py::init([](sim::Id name,
                                         sim::Rpn& rpn,
                                         const sim::Loc& loc) {
                                return sim::PTParameterExpression(
                                    name,
                                    std::move(rpn),
                                    loc);
                             }),
                             py::arg("name"),
                             py::arg("rpn"),
                             py::arg("location") = sim::Loc::bad)
                        .def("name",
                             &sim::PTParameterExpression::name)
                        .def("location",
                             &sim::PTParameterExpression::location)
                        .def("rpn",
                             &sim::PTParameterExpression::rpn,
                             py::return_value_policy::reference_internal)
                        .def("dump",
                             [](const sim::PTParameterExpression& self, int indent) {
                                 self.dump(indent, std::cout);
                             },
                             py::arg("indent"));

                py::class_<sim::PTSweep>(mod_parser_output, "PTSweep")
                        .def(py::init<sim::Id, const sim::Loc&>(),
                             py::arg("name"),
                             py::arg("location") = sim::Loc::bad)
                        .def(py::init([](sim::Id name,
                                         sim::PTParameters &params,
                                         const sim::Loc &loc) {
                            return sim::PTSweep(name, std::move(params), loc);
                        }),
                        py::arg("name"),
                        py::arg("parameters"),
                        py::arg("loc") = sim::Loc::bad)
                        .def_property_readonly("name", &sim::PTSweep::name)
                        .def_property_readonly(
                            "location",
                            &sim::PTSweep::location,
                            py::return_value_policy::reference_internal)
                        .def_property_readonly(
                            "parameters",
                            &sim::PTSweep::parameters,
                            py::return_value_policy::reference_internal)
                        .def(
                            "add",
                            [](sim::PTSweep &self, sim::PTParameters &p) -> sim::PTSweep& {
                                return self.add(std::move(p));
                            },
                            py::return_value_policy::reference_internal
                        )
                        .def(
                            "add",
                            [](sim::PTSweep &self, sim::PTParameterValue &v) -> sim::PTSweep& {
                                return self.add(std::move(v));
                            },
                            py::return_value_policy::reference_internal
                        )
                        .def(
                            "add",
                            [](sim::PTSweep &self, sim::PTParameterExpression &e) -> sim::PTSweep& {
                                return self.add(std::move(e));
                            },
                            py::return_value_policy::reference_internal
                        )
                        .def(
                            "verify",
                            [](const sim::PTSweep& self, int level) {
                                sim::Status s; // TODO
                                return self.verify(level, s);
                            });
                py::class_<sim::PTAnalysis>(mod_parser_output, "PTAnalysis")
                        .def(py::init<>())
                        .def(py::init<sim::Id, sim::Id, const sim::Loc&>(),
                             py::arg("name"),
                             py::arg("type_name"),
                             py::arg("location") = sim::Loc::bad)
                        .def_property_readonly(
                            "location",
                            &sim::PTAnalysis::location,
                            py::return_value_policy::reference_internal)
                        .def_property_readonly(
                            "name",
                            &sim::PTAnalysis::name)
                        .def_property_readonly(
                            "type_name",
                            &sim::PTAnalysis::typeName)
                        .def_property_readonly(
                            "parameters",
                            py::overload_cast<>(&sim::PTAnalysis::parameters),
                            py::return_value_policy::reference_internal)
                        .def_property_readonly(
                            "sweeps",
                            &sim::PTAnalysis::sweeps,
                            py::return_value_policy::reference_internal)
                        .def("add",
                            [](sim::PTAnalysis &self, sim::PTParameters &p) -> sim::PTAnalysis& {
                                return self.add(std::move(p));
                            },
                            py::return_value_policy::reference_internal)
                        
                        .def("add",
                            [](sim::PTAnalysis &self, sim::PTParameterValue &v) -> sim::PTAnalysis& {
                                return self.add(std::move(v));
                            },
                            py::return_value_policy::reference_internal)
                        
                        .def("add",
                            [](sim::PTAnalysis &self, sim::PTParameterExpression &e) -> sim::PTAnalysis& {
                                return self.add(std::move(e));
                            },
                            py::return_value_policy::reference_internal)
                        .def(
                            "verify",
                            [](const sim::PTAnalysis& self, int level) {
                                sim::Status s; // TODO
                                return self.verify(level, s);
                            });
                py::class_<sim::PTSave>(mod_parser_output, "PTSave")
                        .def(py::init<>())
                        .def(py::init<sim::Id, const sim::Loc &>(),
                             py::arg("type_name"),
                             py::arg("loc") = sim::Loc::bad)
                        .def(py::init<sim::Id, sim::Id, const sim::Loc &>(),
                             py::arg("type_name"),
                             py::arg("obj_name"),
                             py::arg("loc") = sim::Loc::bad)
                        .def(py::init<sim::Id, sim::Id, sim::Id, const sim::Loc &>(),
                             py::arg("type_name"),
                             py::arg("obj_name"),
                             py::arg("sub_name"),
                             py::arg("loc") = sim::Loc::bad)
                
                        .def_property_readonly("type_name", &sim::PTSave::typeName)
                        .def_property_readonly("obj_name", &sim::PTSave::objName)
                        .def_property_readonly("sub_name", &sim::PTSave::subName)
                        .def_property_readonly("location", &sim::PTSave::location)
                
                        .def("__repr__", [](const sim::PTSave &s) {
                            std::ostringstream oss;
                            oss << s;
                            return oss.str();
                        });
	}

	{  // pyvacask.compiler
		auto mod_compiler = m.def_submodule("compiler");
                py::class_<sim::OpenvafCompiler, sim::SourceCompiler>(mod_compiler, "OpenvafCompiler")
                        .def(py::init([](
                                std::optional<std::string> compiler,
                                std::optional<std::vector<std::string>> compilerArgs) {
                
                                return sim::OpenvafCompiler(
                                    compiler,
                                    compilerArgs);
                            }),
                            py::arg("compiler") = py::none(),
                            py::arg("compiler_args") = py::none())
                        .def("compile",
                            [](sim::OpenvafCompiler& self,
                               const std::string& loadDirectiveCanonicalPath,
                               const std::string& fileName,
                               const std::string& canonicalPath) {
                
                                std::string outputCanonicalPath;
                                sim::Status status;
                
                                auto [success, cached] =
                                    self.compile(loadDirectiveCanonicalPath,
                                                 fileName,
                                                 canonicalPath,
                                                 outputCanonicalPath,
                                                 status);
                
                                return py::make_tuple(
                                    success,
                                    cached,
                                    outputCanonicalPath);
                            },
                            py::arg("load_directive_canonical_path"),
                            py::arg("file_name"),
                            py::arg("canonical_path"));
	}

	{ // pyvacask.circuit
		auto mod_circuit = m.def_submodule("circuit");
                py::class_<sim::Circuit, std::unique_ptr<sim::Circuit>>(mod_circuit, "Circuit")
                        .def(py::init<sim::ParserTables&, sim::SourceCompiler*>(),
                             py::arg("tables"),
                             py::arg("compiler") = nullptr)
                        .def("is_valid", &sim::Circuit::isValid)
                        .def("needs_elaboration", &sim::Circuit::needsElaboration)
                        .def("clear", &sim::Circuit::clear)
                        .def("title",
                             &sim::Circuit::title,
                             py::return_value_policy::reference_internal)
                        .def("set_title", &sim::Circuit::setTitle)
                        .def("device_count", &sim::Circuit::deviceCount)
                        .def("node_count", &sim::Circuit::nodeCount)
                        .def("unknown_count", &sim::Circuit::unknownCount)
                        .def("instance_count", &sim::Circuit::instanceCount)
                        .def("subcircuit_instance_count", &sim::Circuit::subcircuitInstanceCount)
                        .def("get_variable",
                             [](const sim::Circuit& c, sim::Id name) -> py::object {
                                 auto* v = c.getVariable(name);
                                 if (v)
                                     return py::cast(*v);
                                 return py::none();
                             })
                        .def("set_variable",
                             [](sim::Circuit& c, sim::Id name, const sim::Value& v) {
                                 return c.setVariable(name, v);
                             })
                        .def("elaborate",
                             [](sim::Circuit& c) {
                                 return c.elaborate();
                             })
                        .def("elaborate_changes",
                             [](sim::Circuit& c) {
                                 return c.elaborateChanges(nullptr);
                             });
	}
}
