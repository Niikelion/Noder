#pragma once

#include <Noder/Nodes.hpp>

namespace Noder
{
    class DLLACTION NodeCompiler
    {
    public:
		static void initializeLlvm();

		std::unique_ptr<Enviroment> extractEnviroment();
		std::unique_ptr<Enviroment> swapEnviroment(std::unique_ptr<Enviroment>&&);
		Enviroment& getEnviroment();

		void resetEnviroment();

		void resetFactories();

		NodeCompiler();
		NodeCompiler(std::unique_ptr<Enviroment>&&);
    private:
        std::unique_ptr<Enviroment> env;
    };
}