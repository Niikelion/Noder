#include <Noder/Compiler.hpp>

int main()
{
	using namespace Noder;

	NodeCompiler::initializeLlvm();
	NodeCompiler compiler;
	std::unique_ptr<CompilerTools::Program> program = compiler.generateFunctions();
	std::shared_ptr<CompilerTools::ExecutionEngine> engine = program->getExecutableInstance();
	engine->getSymbol<void()>("printer")();
	program->compile("printer.obj");

	return 0;
}