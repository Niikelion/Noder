#pragma once

#include <Noder/Interpreter.hpp>

namespace Noder
{
	namespace Tools
	{
		class Interpreter
		{
		private:
			NodeInterpreter inter;
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
		};
	}
}