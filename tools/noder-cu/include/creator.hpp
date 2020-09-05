#pragma once

#include <string>

#include <pybind11/pybind11.h>

#include <Noder/Interpreter.hpp>

class Creator
{
private:
	Noder::Enviroment* enviroment;

	pybind11::object locals;
public:
	class PyNodePortWrapper;

	class PyNodeWrapper
	{
	private:
		Noder::Node* node;
	public:
		inline Noder::Node* getNode()
		{
			return node;
		}
		bool isSame(const PyNodeWrapper& target) const;
		std::string getType() const;

		PyNodePortWrapper getFlowInputPort(int port);
		PyNodePortWrapper getFlowOutputPort(int port);
		PyNodePortWrapper getInputPort(int port);
		PyNodePortWrapper getOutputPort(int port);

		PyNodeWrapper(Noder::Node* n) : node(n) {}
	};

	class PyNodePortWrapper
	{
	private:
		Noder::Node::PortWrapper port;
	public:
		PyNodeWrapper getNode();
		unsigned getPort();
		Noder::Node::PortTypeWrapper::Type getType();

		void setValue(const std::string& value);
		void connect(const PyNodePortWrapper& target);

		PyNodePortWrapper(Noder::Node::PortWrapper p) : port(p) {}
	};

	class PyNodeEnviromentWrapper
	{
	private:
		Noder::Enviroment* enviroment;
		std::map<std::string, Noder::NodeTemplate*> templates;
		std::map<std::string, Noder::Node*> nodes;
	public:
		void removeNode(PyNodeWrapper node);
		pybind11::object getNode(size_t id);
		pybind11::object findNode(const std::string& id);
		pybind11::object createNode(const std::string& type, const std::string& name);
		PyNodeEnviromentWrapper(Noder::Enviroment* env) : enviroment(env)
		{
			size_t i = 0;
			for (auto& node : env->nodes)
			{
				nodes.emplace(std::to_string(i),node.get());
				++i;
			}
			for (auto& type : env->nodeTemplates)
			{
				templates.emplace(type->action, type.get());
			}
		}
	};

	void run();

	Creator(Noder::Enviroment* env) : enviroment(env) {}
};