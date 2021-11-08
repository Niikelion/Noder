#include <catch.hpp>
#include <cmath>
#include <string>
#include <tuple>
#include <sstream>

#include <iostream>

#include <Noder/Compiler.hpp>

TEST_CASE("Compiler - temporary test", "[unit],[compiler]")
{
	using namespace Noder;
	NodeCompiler::initializeLlvm();
	NodeCompiler compiler;

	//TODO: create program
	Node* node = nullptr;

	auto program = compiler.generateFunction(*node,"test");
	auto engine = program->getExecutableInstance();
	engine->getSymbol<int()>("test")();

	/*
	using namespace Noder;
	NodeCompiler::initializeLlvm();
	NodeCompiler compiler;
	std::unique_ptr<NodeCompiler::Program> program = compiler.generate();
	std::shared_ptr<NodeCompiler::ExecutionEngine> engine = program->getExecutableInstance();
	engine->getSymbol<void()>("printer")();
	program->compile("printer.obj");
	*/
}