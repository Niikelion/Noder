#include <creator.hpp>
#include <pybind11/embed.h>
namespace py = pybind11;
using namespace py::literals;
#include <iostream>

PYBIND11_EMBEDDED_MODULE(noder, m)
{
	py::class_<Creator::PyNodeWrapper>(m, "Node")
		.def(py::init<Noder::Node*>())
		.def_property_readonly("type", &Creator::PyNodeWrapper::getType)
		.def("getFlowInput", &Creator::PyNodeWrapper::getFlowInputPort)
		.def("getFlowOutput", &Creator::PyNodeWrapper::getFlowOutputPort)
		.def("getValueInput", &Creator::PyNodeWrapper::getInputPort)
		.def("getValueOutput", &Creator::PyNodeWrapper::getOutputPort)
		.def("isSame", &Creator::PyNodeWrapper::isSame)
		;

	py::class_<Creator::PyNodeEnviromentWrapper>(m, "NodeEnviroment")
		.def(py::init<Noder::Enviroment*>())
		.def("getNode", &Creator::PyNodeEnviromentWrapper::getNode)
		.def("findNode", &Creator::PyNodeEnviromentWrapper::findNode)
		.def("createNode", &Creator::PyNodeEnviromentWrapper::createNode, py::arg("type"), py::arg("name") = "")
		;

	py::class_<Creator::PyNodePortWrapper> portWrapper(m, "NodePortWrapper");
	portWrapper
		.def(py::init<Noder::Node::PortWrapper>())
		.def("getNode", &Creator::PyNodePortWrapper::getNode)
		.def("getPort", &Creator::PyNodePortWrapper::getPort)
		.def("getType", &Creator::PyNodePortWrapper::getType)
		.def("connect", &Creator::PyNodePortWrapper::connect)
		.def("setValue", &Creator::PyNodePortWrapper::setValue)
		;

	py::enum_<Noder::Node::PortTypeWrapper::Type>(portWrapper, "Type")
		.value("FlowInput", Noder::Node::PortTypeWrapper::Type::FlowInput)
		.value("FlowOutput", Noder::Node::PortTypeWrapper::Type::FlowOutput)
		.value("ValueInput", Noder::Node::PortTypeWrapper::Type::ValueInput)
		.value("ValueOutput", Noder::Node::PortTypeWrapper::Type::ValueOutput)
		;
}

void Creator::run()
{
	py::scoped_interpreter guard{};

	py::object scope = py::module::import("__main__").attr("__dict__");

	auto module = py::module::import("noder");
	auto locals = py::dict("env"_a = PyNodeEnviromentWrapper(enviroment));
	std::string n;
	while (std::getline(std::cin, n))
	{
		try
		{
			py::object ret = py::eval<py::eval_single_statement>(n, scope, locals);
			/*
			if (!ret.is_none())
			{
				py::print("<", ret.get_type().attr("__name__"), ">: ", ret, "sep"_a = "");
			}
			*/
		}
		catch (py::error_already_set& e)
		{

			if (e.matches(PyExc_SystemExit))
			{
				return;
			}
			std::cout << e.what() << std::endl;
		}
	}
}

void Creator::PyNodeEnviromentWrapper::removeNode(PyNodeWrapper node)
{
	Noder::Node* n = node.getNode();
	for (auto i = enviroment->nodes.begin(); i != enviroment->nodes.end(); ++i)
	{
		if (i->get() == n)
		{
			//TODO: remove connections and name mapping, maybe move some code to enviroment class?
			enviroment->nodes.erase(i);
		}
	}
}

py::object Creator::PyNodeEnviromentWrapper::getNode(size_t id)
{
	if (id < enviroment->nodes.size())
		return py::cast<PyNodeWrapper>(PyNodeWrapper(enviroment->nodes[id].get()));
	return py::none();
}

py::object Creator::PyNodeEnviromentWrapper::findNode(const std::string& id)
{
	auto it = nodes.find(id);
	if (it != nodes.end())
	{
		return py::cast<PyNodeWrapper>(PyNodeWrapper(it->second));
	}
	return py::none();
}

pybind11::object Creator::PyNodeEnviromentWrapper::createNode(const std::string& type, const std::string& name)
{
	auto it = templates.find(type);
	if (it != templates.end())
	{
		Noder::Node* ret = &enviroment->createNode(*it->second);
		if (name.length() > 0)
			nodes.emplace(name,ret);
		return py::cast<PyNodeWrapper>(PyNodeWrapper(ret));
	}
	return pybind11::none();
}

bool Creator::PyNodeWrapper::isSame(const PyNodeWrapper& target) const
{
	return target.node == node;
}

std::string Creator::PyNodeWrapper::getType() const
{
	return node->getBase()->action;
}

Creator::PyNodePortWrapper Creator::PyNodeWrapper::getFlowInputPort(int port)
{
	return node->getFlowInputPort(port);
}

Creator::PyNodePortWrapper Creator::PyNodeWrapper::getFlowOutputPort(int port)
{
	return node->getFlowOutputPort(port);
}

Creator::PyNodePortWrapper Creator::PyNodeWrapper::getInputPort(int port)
{
	return node->getInputPort(port);
}

Creator::PyNodePortWrapper Creator::PyNodeWrapper::getOutputPort(int port)
{
	return node->getOutputPort(port);
}

Creator::PyNodeWrapper Creator::PyNodePortWrapper::getNode()
{
	return port.getNode();
}

unsigned Creator::PyNodePortWrapper::getPort()
{
	return port.getPort();
}

Noder::Node::PortTypeWrapper::Type Creator::PyNodePortWrapper::getType()
{
	using Type = Noder::Node::PortTypeWrapper::Type;
	if (port.isFlowPort())
	{
		//flow ports
		if (port.isInput())
			return Type::FlowInput;
		else
			return Type::FlowOutput;
	}
	else
	{
		//value ports
		if (port.isInput())
			return Type::ValueInput;
		else
			return Type::ValueOutput;
	}
	return Type::FlowInput;
}

void Creator::PyNodePortWrapper::setValue(const std::string& value)
{
	if (getType() == Noder::Node::PortTypeWrapper::Type::ValueInput)
	{
		port.getNode()->deserializeInputValue(port.getPort(), value);
	}
}

void Creator::PyNodePortWrapper::connect(const PyNodePortWrapper& target)
{
	port.connect(target.port);
}
