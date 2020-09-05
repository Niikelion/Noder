#pragma once

#include <Noder/Interpreter.hpp>
#include <slib.hpp>
#include <string>
#include <unordered_map>

namespace Noder
{
	namespace Tools
	{
		using Module = Nlib::SharedLib;
		class Interpreter
		{
		private:
			NodeInterpreter inter;
			std::unordered_map<std::string, Module> modules;
		public:
			void loadDefaultDefinitions();
			void loadDefinitions(const std::string& module);

			void loadEnviromentFromXmlFile(const std::string& file);
			void saveEnviromentToXmlFile(const std::string& file);

			Node* findStartNode();

			Node* getNode(unsigned n);

			Enviroment* getEnviroment();

			void runFrom(Node& node);
			void run();

			void prepare();

			~Interpreter();
		};
	}
}