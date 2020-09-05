#include <Interpreter.hpp>
#include <fstream>
#include <cmath>
#include "Interpreter.hpp"

#include <config.hpp>

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
			//TODO: move to cu-interpreter
			typedef float(*funcf2f)(float);
			typedef float(*funcff2f)(float, float);

			inter.createTemplate([]() {return std::tuple<>(); }, "start").flowOutputPoints = 1;
			NodeTemplate& printT = inter.createTemplate([](std::string value) { printf("%s\n", value.c_str()); }, "print");
			printT.flowInputPoints = 1;
			printT.flowOutputPoints = 1;


			inter.createTemplate((funcff2f)pow, "pow");
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
	}
}