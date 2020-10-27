#include <Noder/Compiler.hpp>

int main()
{
	using namespace Noder;

	NodeCompiler::initializeLlvm();
	NodeCompiler compiler;
	std::unique_ptr<NodeCompiler::Program> program = compiler.generate();
	std::shared_ptr<NodeCompiler::ExecutionEngine> engine = program->getExecutableInstance();
	engine->getSymbol<void()>("printer")();
	program->compile("printer.obj");

	return 0;
}