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
	auto printerTemplate = compiler.createTemplate<void()>([](
		const Node& entry,
		CompilerTools::LlvmBuilder& builder,
		CompilerTools::LlvmBuilder::Function function,
		std::vector<CompilerTools::LlvmBuilder::InstructionBlock> flowInputs,
		std::vector<CompilerTools::LlvmBuilder::InstructionBlock>& flowOutputs,
		std::vector<CompilerTools::LlvmBuilder::Value> inputs,
		std::vector<CompilerTools::LlvmBuilder::Value>& outputs)
		{
			builder.setInsertPoint(flowInputs[0]);
			CompilerTools::LlvmBuilder::Value str = builder.createCString("Hello world!\n");

			CompilerTools::LlvmBuilder::ExternalFunction putsFunc("puts", CompilerTools::LlvmBuilder::Type::get<int32_t(int8_t*)>(builder.getBuilder().getContext()), *function.getFunction()->getParent());
			builder.addFunctionCall(putsFunc, { str });
			builder.addVoidReturn();
			flowOutputs.emplace_back(flowInputs[0]);
		}, "printer", 1, 1);
	auto node = compiler.createNode(printerTemplate);

	auto program = compiler.generateFunction(*node,"test");
	auto engine = program->getExecutableInstance();
	engine->getSymbol<void()>("test")();

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