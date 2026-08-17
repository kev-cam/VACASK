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

// pyvacask.parser_output
void InitParserOutputModule(py::module &m) {
    py::class_<sim::ParserTables>(m, "ParserTables")
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

        py::class_<sim::PTLoad>(m, "PTLoad")
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

        py::class_<sim::PTModel>(m, "PTModel")
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
                m, "PTSubcircuitDefinition")
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
                //.def("root",
                //        &sim::PTSubcircuitDefinition::root,
                //        py::return_value_policy::reference_internal)
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



            py::class_<sim::PTInstance>(m, "PTInstance")
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

            py::class_<sim::PTParameters>(m, "PTParameters")
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

            py::class_<sim::PTParameterValue>(m, "PTParameterValue")
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

            py::class_<sim::PTParameterExpression>(m, "PTParameterExpression")
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
                //.def("rpn",
                //        &sim::PTParameterExpression::rpn,
                //        py::return_value_policy::reference_internal)
                .def("dump",
                        [](const sim::PTParameterExpression& self, int indent) {
                            self.dump(indent, std::cout);
                        },
                        py::arg("indent"));

            py::class_<sim::PTSweep>(m, "PTSweep")
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

            py::class_<sim::PTAnalysis>(m, "PTAnalysis")
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

            py::class_<sim::PTSave>(m, "PTSave")
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

            py::class_<sim::PTParsedIdentifier>(m, "PTParsedIdentifier")
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
        
      