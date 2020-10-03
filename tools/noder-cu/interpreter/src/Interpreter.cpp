#include <fstream>
#include <cmath>
#include "Interpreter.hpp"

namespace Noder
{
	namespace Tools
	{
		void Interpreter::loadDefaultDefinitions()
		{
			loadDefinitions("cu-interpreter-base");
		}
		void Interpreter::loadDefinitions(const std::string& module)
		{
			if (modules.count(module))
				throw std::runtime_error("Module already loaded.");
			Module m;
			if (!m.load("modules/" + module))
				throw std::runtime_error("Could not load module.");

			auto f = m.getFunction<void, Noder::NodeInterpreter&>("exportActions");
			if (!f)
				throw std::runtime_error("Could not find exportActions signature");

			f(inter);

			modules.emplace(module, std::move(m));
		}
		void Interpreter::loadEnviromentFromXmlFile(const std::string& file)
		{
			std::ifstream input(file);

			if (input.is_open())
			{
				std::string str;

				input.seekg(0, std::ios::end);
				str.reserve(input.tellg());
				input.seekg(0, std::ios::beg);

				str.assign((std::istreambuf_iterator<char>(input)),
					std::istreambuf_iterator<char>());

				auto res = XML::parse(str, true);
				if (res)
					inter.getEnviroment().importFromXML(res);
			}
		}
		void Interpreter::saveEnviromentToXmlFile(const std::string& file)
		{
			std::ofstream output(file);
			if (output.is_open())
			{
				output << inter.getEnviroment().exportToXML()->format(false);
			}
		}
		Node* Interpreter::findStartNode()
		{
			Node* ret = nullptr;

			for (auto& node : inter.getEnviroment().nodes)
			{
				if (node->getBase()->action == "start")
				{
					ret = node.get();
					break;
				}
			}

			return ret;
		}
		Node* Interpreter::getNode(unsigned n)
		{
			if (n < inter.getEnviroment().nodes.size())
			{
				return inter.getEnviroment().nodes[n].get();
			}
			return nullptr;
		}
		Enviroment* Interpreter::getEnviroment()
		{
			return &inter.getEnviroment();
		}
		void Interpreter::runFrom(Node& node)
		{
			inter.runFrom(node);
		}
		void Interpreter::run()
		{
			Node* start = findStartNode();
			if (start != nullptr)
			{
				runFrom(*start);
			}
		}
		void Interpreter::prepare()
		{
			inter.buildStates();
		}
		Interpreter::~Interpreter()
		{
			getEnviroment()->clear();
			inter.resetFactories();
		}
	}
}