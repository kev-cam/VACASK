#define PYBIND11_DETAILED_ERROR_MESSAGES
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/operators.h>
#include <simulator.h>
#include <circuit.h>
#include <openvafcomp.h>
#include <parser.h>
#include <sourceloc.h>

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
	
        // pyvacask.elsetup
        {
		auto mod_elsetup = m.def_submodule("elsetup");
                py::class_<sim::DeviceRequests>(mod_elsetup, "DeviceRequests");
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
        
        { // pyvacask.loc
            auto mod_loc = m.def_submodule("loc");
        
            py::class_<sim::Loc>(mod_loc, "Loc")
                .def(py::init<>())
                .def(py::init<
                         sim::FileStackIndex,
                         sim::FileStackFileIndex,
                         sim::SourceLineNumber,
                         sim::SourceColumnNumber>(),
                     py::arg("file_stack"),
                     py::arg("file"),
                     py::arg("line"),
                     py::arg("column"))
                .def("data", &sim::Loc::data)
                .def("toString", &sim::Loc::toString)
                .def("__str__", &sim::Loc::toString)
                .def("__repr__",
                     [](const sim::Loc &self) {
                         return "Loc('" + self.toString() + "')";
                     })
                .def("__bool__",
                     [](const sim::Loc &self) {
                         return static_cast<bool>(self);
                     })
                .def("__eq__",
                     [](const sim::Loc &a, const sim::Loc &b) {
                         return a == b;
                     })
                .def("__ne__",
                     [](const sim::Loc &a, const sim::Loc &b) {
                         return a != b;
                     })
                .def_readonly_static("bad", &sim::Loc::bad);
        }

        { // pyvacask.id
            auto mod_id = m.def_submodule("id");
            py::class_<sim::Id>(mod_id, "Id")
                // Constructors
                .def(py::init<>())
                .def(py::init<sim::IdentifierIndex>(),
                     py::arg("id"))
                .def(py::init<const std::string &>(),
                     py::arg("name"))
                .def_property_readonly("id", &sim::Id::id)
                .def("c_str", &sim::Id::c_str)
                .def("__str__",
                     [](const sim::Id &self) {
                         return std::string(self);
                     })
                .def("__repr__",
                     [](const sim::Id &self) {
                         return "<Id '" + std::string(self) + "'>";
                     })
                .def("__bool__",
                     [](const sim::Id &self) {
                         return static_cast<bool>(self);
                     })
                .def(py::self == py::self)
                .def(py::self != py::self)
                .def_static("createStatic",
                            &sim::Id::createStatic,
                            py::arg("name"))
                .def_readonly_static("none", &sim::Id::none);
        }

        { // pyvacsk.value
		auto mod_value = m.def_submodule("value");
                py::class_<sim::Value>(mod_value, "Value")
                        .def(py::init<>())
                        .def(py::init<sim::Int>())
                        .def(py::init<sim::Real>())
                        .def(py::init<const sim::String &>())
                        .def(py::init<const char *>())
                        .def(py::init<const sim::IntVector &>())
                        .def(py::init<const sim::RealVector &>())
                        .def(py::init<const sim::StringVector &>())
                        .def(py::init<const sim::ValueVector &>());
        }

	{ // pyvacask.parser_output
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
                        )
                        .def("verify",
                            [](sim::ParserTables& self, sim::Status& s) -> bool { return self.verify(s); },
                            py::return_value_policy::reference_internal
                        )
                        .def("writeEmbedded",
                            [](sim::ParserTables& self, int debug, sim::Status& s) -> bool { 
                                return self.writeEmbedded(debug, s);
                            },
                            py::return_value_policy::reference_internal
                        );

                py::class_<sim::PTLoad>(mod_parser_output, "PTLoad")
                        .def(py::init<>())
                        .def(
                            py::init([](const std::string& file, const sim::Loc& loc) {
                                return sim::PTLoad(file, loc);
                            }),
                            py::arg("file"),
                            py::arg("location") = sim::Loc::bad
                        )
                        .def(py::init([](const std::string &file,
                                         sim::PTParameters &par,
                                         const sim::Loc &loc) {
                            return sim::PTLoad(file, std::move(par), loc);
                        }),
                        py::arg("file"),
                        py::arg("parameters"),
                        py::arg("location") = sim::Loc::bad)
                        // Getters
                        .def(
                            "location",
                            &sim::PTLoad::location,
                            py::return_value_policy::reference_internal
                        )
                        .def(
                            "file",
                            &sim::PTLoad::file,
                            py::return_value_policy::reference_internal
                        )
                        .def(
                            "parameters",
                            [](sim::PTLoad& self) -> sim::PTParameters& {
                                return const_cast<sim::PTParameters&>(self.parameters());
                            },
                            py::return_value_policy::reference_internal
                        )
                        .def(
                            "add",
                            [](sim::PTLoad& self, sim::PTParameters& par) -> sim::PTLoad& {
                                return self.add(std::move(par));
                            },
                            py::arg("parameters"),
                            py::return_value_policy::reference_internal
                        )
                        
                        .def(
                            "add",
                            [](sim::PTLoad& self, sim::PTParameterValue& v) -> sim::PTLoad& {
                                return self.add(std::move(v));
                            },
                            py::arg("value"),
                            py::return_value_policy::reference_internal
                        )
                        .def(
                            "verify",
                            [](const sim::PTLoad& self, int level, py::object obj) {
                                if (obj.is_none())
                                    return self.verify(level);
                                return self.verify(level, obj.cast<sim::Status&>());
                            },
                            py::arg("level"),
                            py::arg("status") = py::none()
                        )
                        .def(
                            "dump",
                            [](const sim::PTLoad& self, int indent) {
                                std::ostringstream os;
                                self.dump(indent, os);
                                return os.str();
                            },
                            py::arg("indent")
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
                py::class_<sim::PTParsedIdentifier>(mod_parser_output, "PTParsedIdentifier")
                        .def(py::init<const char*, sim::Loc>(),
                             py::arg("name"),
                             py::arg("location") = sim::Loc::bad)
                        .def(py::init<sim::Id, sim::Loc>(),
                             py::arg("name"),
                             py::arg("location") = sim::Loc::bad)
                        .def("name",
                             &sim::PTParsedIdentifier::name)
                        .def("location",
                             &sim::PTParsedIdentifier::location)
                        .def("__repr__",
                             [](const sim::PTParsedIdentifier& self) {
                                 std::ostringstream os;
                                 os << self;
                                 return os.str();
                             });
	}

        { // pyvacask.parser
		auto mod_parser = m.def_submodule("parser");
                py::class_<sim::Parser>(mod_parser, "Parser")
                        .def(py::init<sim::ParserTables&>(),
                             py::arg("tables"),
                             py::keep_alive<1, 2>())
                        .def("parseNetlistFile",
                             [](sim::Parser &self, sim::FileStackFileIndex fileIndex) {
                                 return self.parseNetlistFile(fileIndex);
                             },
                             py::arg("fileIndex"))
                        .def("parseNetlistString",
                             [](sim::Parser &self, const std::string &input) {
                                 return self.parseNetlistString(input);
                             },
                             py::arg("input"))
                        .def("parseExpression",
                             [](sim::Parser &self, const std::string &input) {
                                 return self.parseExpression(input);
                             },
                             py::arg("input"))
                        .def("parseParameters",
                             [](sim::Parser &self, const std::string &input) {
                                 return self.parseParameters(input);
                             },
                             py::arg("input"));
        }
	
        { // pyvacask.circuit
		auto mod_circuit = m.def_submodule("circuit");
                py::class_<sim::Circuit, std::unique_ptr<sim::Circuit>>(mod_circuit, "Circuit")
                        .def(py::init<sim::ParserTables&, sim::SourceCompiler*, sim::Status&>(),
                             py::arg("tables"),
                             py::arg("compiler") = nullptr,
                             py::arg("status"),
                             py::return_value_policy::reference_internal)
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
                        .def("setOption",
                             [](sim::Circuit& c, sim::Id name, const sim::Value& v) {
                                 sim::Status s; // TODO
                                 return c.setOption(name, v, s);
                             })
                        .def(
                            "elaborate",
                            [](sim::Circuit &self,
                               const std::vector<sim::Id> &defs,
                               const std::string &defName,
                               const std::string &instName,
                               sim::DeviceRequests *devReq,
                               sim::Status &status)
                            {
                                return self.elaborate(defs, defName, instName, devReq, status);
                            },
                            py::arg("toplevel_definitions") = std::vector<sim::Id>{},
                            py::arg("top_def_name") = "__topdef__",
                            py::arg("top_inst_name") = "__topinst__",
                            py::arg("dev_req") = nullptr,
                            py::arg("status") 
                        )
                        .def("dumpDevices",                                   
                            [](const sim::Circuit& self, int indent) {
                                self.dumpDevices(indent, std::cout);
                            },
                            py::arg("indent")) 
                        .def("dumpModels",                                   
                            [](const sim::Circuit& self, int indent) {
                                self.dumpModels(indent, std::cout);
                            },
                            py::arg("indent")) 
                        .def("dumpVariables",                                   
                            [](const sim::Circuit& self, int indent) {
                                self.dumpVariables(indent, std::cout);
                            },
                            py::arg("indent"))
                        .def("dumpOptions",                               
                            [](const sim::Circuit& self, int indent) {
                                self.dumpOptions(indent, std::cout);
                            },
                            py::arg("indent"))
                        .def("dumpHierarchy",
                            [](const sim::Circuit& self, int indent) {
                                self.dumpHierarchy(indent, std::cout);
                            },
                            py::arg("indent"))
                        .def("dumpNodes",
                            [](const sim::Circuit& self, int indent) {
                                self.dumpNodes(indent, std::cout);
                            },
                            py::arg("indent"))
                        .def("dumpUnknowns",
                            [](const sim::Circuit& self, int indent) {
                                self.dumpUnknowns(indent, std::cout);
                            },
                            py::arg("indent"))
                        .def("dumpSparsity",
                            [](const sim::Circuit& self, int indent) {
                                self.dumpSparsity(indent, std::cout);
                            },
                            py::arg("indent"))
                        .def("dumpDeviceCounts",
                            [](const sim::Circuit& self, int indent) {
                                self.dumpDeviceCounts(indent, std::cout);
                            },
                            py::arg("indent"));


                py::class_<sim::SourceCompiler>(mod_circuit, "SourceCompiler");
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

        { // pyvacask.analysis
		auto mod_analysis = m.def_submodule("analysis");
                py::class_<sim::Analysis>(mod_analysis, "Analysis")
                    .def_property_readonly("name", &sim::Analysis::name)
                    .def("sweep_count", &sim::Analysis::sweepCount)
                    .def("update_sweeper", &sim::Analysis::updateSweeper)
                    .def(
                        "add",
                        py::overload_cast<const sim::PTSave &>(&sim::Analysis::add),
                        py::return_value_policy::reference_internal
                    )
                    .def(
                        "add",
                        py::overload_cast<const sim::PTSaves &>(&sim::Analysis::add),
                        py::return_value_policy::reference_internal
                    )
                    .def(
                        "add",
                        py::overload_cast<const sim::PTParameters &>(&sim::Analysis::add),
                        py::return_value_policy::reference_internal
                    )
                    .def(
                        "add",
                        [](sim::Analysis &self, sim::PTParameters &p) -> sim::Analysis& {
                            return self.add(std::move(p));
                        },
                        py::return_value_policy::reference_internal
                    )
                    .def(
                        "add",
                        py::overload_cast<const sim::PTParameterMap &>(&sim::Analysis::add),
                        py::return_value_policy::reference_internal
                    )
                    .def_static(
                        "create",
                        &sim::Analysis::create,
                        py::arg("pt_analysis"),
                        py::arg("circuit"),
                        py::arg("status"),
                        py::return_value_policy::take_ownership
                    )
                    .def("start", &sim::Analysis::start)
                    .def("is_running", &sim::Analysis::isRunning)
                    .def("resume", &sim::Analysis::resume)
                    .def("finish", &sim::Analysis::finish)
                    .def("run", &sim::Analysis::run)
                    .def("update_parameter_expressions", &sim::Analysis::updateParameterExpressions)
                    .def("requests_rebuild", &sim::Analysis::requestsRebuild)
                    .def("pre_mapping", &sim::Analysis::preMapping)
                    .def("populate_structures", &sim::Analysis::populateStructures);
        }

        { // rpnexpr
		auto mod_rpnexpr = m.def_submodule("rpnexpr");
                py::class_<sim::Rpn>(mod_rpnexpr, "Rpn");
        }
}
