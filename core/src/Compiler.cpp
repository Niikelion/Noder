#include "Noder/Compiler.hpp"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/Support/ToolOutputFile.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/CodeGen/LinkAllAsmWriterComponents.h"
#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/CodeGen/MIRParser/MIRParser.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/AutoUpgrade.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LLVMRemarkStreamer.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/InitializePasses.h"
#include "llvm/MC/SubtargetFeature.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/WithColor.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/Target/TargetMachine.h"

#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Scalar.h"

#include <memory>
#include <iostream>
#include <unordered_set>

#include <cassert>

namespace Noder
{
	std::unique_ptr<CompilerTools::Program> NodeCompiler::generateFunctions()
	{
		using namespace CompilerTools;

		LlvmBuilder builder(context);
		std::unique_ptr<Program> program = std::make_unique<Program>(std::make_unique<llvm::Module>("definitions",context));
		program->getModule().setTargetTriple(llvm::sys::getDefaultTargetTriple());

		LlvmBuilder::Function function("printer", LlvmBuilder::Type::get<void()>(context), *program);
		LlvmBuilder::InstructionBlock entryPoint(function,"entry");

		builder.setInsertPoint(entryPoint);

		LlvmBuilder::Value str = builder.createCString("Hello world!\n");
		
		LlvmBuilder::ExternalFunction putsFunc("puts", LlvmBuilder::Type::get<int32_t(int8_t*)>(context), *program);
		builder.addFunctionCall(putsFunc, {str});

		builder.addVoidReturn();

		return program;
	}
	std::unique_ptr<CompilerTools::Program> NodeCompiler::generateAll(Noder::Node& mainEntry)
	{
		using namespace CompilerTools;

		LlvmBuilder builder(context);
		std::unique_ptr<Program> program = std::make_unique<Program>(std::make_unique<llvm::Module>("main", context));
		program->getModule().setTargetTriple(llvm::sys::getDefaultTargetTriple());

		LlvmBuilder::Function function("main", LlvmBuilder::Type::get<int()>(context), *program);
		LlvmBuilder::InstructionBlock entryPoint(function, "entry");

		//

		return program;
	}
	void NodeCompiler::initializeLlvm()
	{
		llvm::InitializeAllTargetInfos();
		llvm::InitializeAllTargets();
		llvm::InitializeAllTargetMCs();
		llvm::InitializeAllAsmParsers();
		llvm::InitializeAllAsmPrinters();

		llvm::InitializeNativeTarget();
		llvm::InitializeNativeTargetAsmPrinter();
	}
	std::unique_ptr<Enviroment> NodeCompiler::extractEnviroment()
	{
		return std::move(env);
	}
	std::unique_ptr<Enviroment> NodeCompiler::swapEnviroment(std::unique_ptr<Enviroment>&& t)
	{
		std::unique_ptr<Enviroment> tmp = std::move(env);
		env = std::move(t);
		return std::move(tmp);
	}
	Enviroment& NodeCompiler::getEnviroment()
	{
		return *env;
	}
	void NodeCompiler::resetEnviroment()
	{
		env.reset();
	}
	void NodeCompiler::resetFactories()
	{
		stateFactories.clear();
	}
	NodeCompiler::NodeCompiler()
	{
		env = std::make_unique<Enviroment>();
	}
	NodeCompiler::NodeCompiler(std::unique_ptr<Enviroment>&& t)
	{
		env = std::move(t);
		//TODO: setup builders
	}

	std::vector<CompilerTools::LlvmBuilder::InstructionBlock> NodeCompiler::generateNode(
		const Node& node,
		CompilerTools::LlvmBuilder& builder,
		std::vector<CompilerTools::LlvmBuilder::InstructionBlock> entries,
		std::vector<CompilerTools::Value> inputs,
		std::vector<CompilerTools::Value>& outputs)
	{
		std::vector<CompilerTools::LlvmBuilder::InstructionBlock> ret;
		auto it = states.find(&node);
		if (it == states.end())
		{
			//TODO: error handling-state missing
		}
		
		auto base = node.getBase();
		/*
		std::vector<CompilerTools::Value> inputs;
		inputs.resize(base->outputs.size());
		
		//gather inputs
		for (size_t i = 0; i < inputs.size(); ++i)
		{
			auto port = node.getInputPortTarget(i);
			if (port.isVoid())
			{
				//TODO: handle const value
				//port not connected
			}
			else
			{
				const auto& n = port.getNode();
				auto it2 = states.find(&n);
				if (it2 == states.end())
				{
					//TODO: throw, missing node state
				}
				//TODO: check for invalid index
				CompilerTools::Value* v = it2->second->outputs[port.getPort()].get();
				if (v == nullptr)
				{
					//TODO: throw, output not generated yet
				}
				inputs[i] = *v;
			}
		}
		*/
		it->second->generate(node, builder, entries, ret, inputs, outputs);

		return ret;
	}

	void NodeCompiler::generateInstruction(
		const Node& entry,
		CompilerState& state,
		std::vector<CompilerTools::LlvmBuilder::InstructionBlock> flowInputs,
		std::vector<CompilerTools::LlvmBuilder::InstructionBlock>& flowOutputs)
	{
		using namespace CompilerTools;
		for (int i = 0; i <= 0; ++i)
		{
			std::vector<const Noder::Node*> nodesToCalculate;
			nodesToCalculate.push_back(&entry);
			state.visitedNodes.insert(&entry);

			while (!nodesToCalculate.empty())
			{
				const Noder::Node* n = nodesToCalculate.back();
				bool depsAlreadyResolved = true;

				std::unordered_set<const Noder::Node*> neededNodes;

				const auto base = n->getBase();

				assert(base != nullptr);

				for (int i = 0; i < base->inputs.size(); ++i)
				{
					auto port = n->getInputPort(i);
					if (!port.isVoid())
					{
						const Noder::Node& t = port.getNode();
						neededNodes.insert(&t);
					}
				}

				for (auto& dep : neededNodes)
				{
					if (!state.calculatedNodes.count(dep))
					{
						if (state.visitedNodes.count(dep) > 0)
							throw std::logic_error("Cyclic dependency detected");
						depsAlreadyResolved = false;
						nodesToCalculate.push_back(dep);
						state.visitedNodes.insert(dep);
					}
				}
				if (depsAlreadyResolved)
				{
					nodesToCalculate.pop_back();
					std::vector<Value> inputs, outputs;
					inputs.resize(base->inputs.size());

					auto nodeStateIt = states.find(n);
					if (nodeStateIt == states.end())
					{
						//TODO: throw exception, missing state
					}

					for (int i = 0; i < inputs.size(); ++i)
					{
						//TODO: gather inputs
					}

					nodeStateIt->second->generate(*n, state.builder, flowInputs, flowOutputs, inputs, outputs);

					state.nodeOutputs.emplace(n, std::move(outputs));

					state.calculatedNodes.insert(n);
				}
			}
		}
	}

	void NodeCompiler::generateFull(const Node& entry, CompilerTools::LlvmBuilder& builder)
	{
		using namespace CompilerTools;
		LlvmBuilder::InstructionBlock entryBlock();
		//
	}

	void* CompilerTools::ExecutionEngine::getSymbol(const std::string& name)
	{
		return engine->getPointerToNamedFunction(name);
	}
	CompilerTools::ExecutionEngine::ExecutionEngine(const llvm::Module& m)
	{
		std::string errStr;
		std::unique_ptr<llvm::Module> module = llvm::CloneModule(m);

		engine.reset(llvm::EngineBuilder(std::move(module)).setErrorStr(&errStr).create());

		if (!engine)
		{
			std::cout << "err:" << errStr << std::endl;
		}
		else
		{
			engine->finalizeObject();
		}
	}
	void CompilerTools::Program::compile(const std::string& filename)
	{
		std::error_code errCode;

		llvm::raw_fd_ostream out(filename, errCode, llvm::sys::fs::OF_None);

		if (errCode)
		{
			//error
			return;
		}

		llvm::legacy::PassManager passManager;

		std::string error;

		const llvm::Target* theTarget = llvm::TargetRegistry::lookupTarget(module->getTargetTriple(),error);

		if (theTarget == nullptr)
		{
			//error
			return;
		}

		//TODO: move passmanager to shared instance
		//TODO: add option to specify optimalization level
		
		llvm::TargetOptions options;

		std::unique_ptr<llvm::TargetMachine> target = std::unique_ptr<llvm::TargetMachine>(theTarget->createTargetMachine(
			module->getTargetTriple(),
			llvm::sys::getHostCPUName(),
			"",
			options,
			{},
			{},
			llvm::CodeGenOpt::Default
		));
		
		if (!target)
		{
			//error
			return;
		}
		
		//passManager.add(llvm::createPromoteMemoryToRegisterPass());
		//passManager.add(llvm::createCFGSimplificationPass());

		if (target->addPassesToEmitFile(
			passManager,
			out,
			nullptr,
			llvm::CodeGenFileType::CGFT_ObjectFile
		))
		{
			//error: file type not supported
		}
		else
		{
			std::unique_ptr<llvm::Module> copy = llvm::CloneModule(*module);
			copy->setDataLayout(target->createDataLayout().getStringRepresentation());
			passManager.run(*copy);
			out.flush();
			//
		}
	}
	CompilerTools::LlvmBuilder::Value CompilerTools::LlvmBuilder::createCString(const std::string& value)
	{
		return Value(builder.CreateGlobalStringPtr(value));
	}
	void CompilerTools::LlvmBuilder::addFunctionCall(const Callee& function, const std::vector<Value>& arguments)
	{
		std::vector<llvm::Value*> args;

		args.reserve(arguments.size());

		for (const auto& arg : arguments)
			args.push_back(arg.value);

		builder.CreateCall(function.getHandle(), args, function.getName());
	}
	void CompilerTools::LlvmBuilder::addReturn(const Value& value)
	{
		builder.CreateRet(value.value);
	}
	void CompilerTools::LlvmBuilder::addVoidReturn()
	{
		builder.CreateRetVoid();
	}
	void CompilerTools::LlvmBuilder::setInsertPoint(InstructionBlock& block)
	{
		builder.SetInsertPoint(block.getBlock());
	}

	std::unique_ptr<CompilerTools::Program> Noder::CompilerTools::StructureBuilder::generate(llvm::LLVMContext& context, const Noder::Node& entry, const std::string& fun)
	{
		
		using namespace CompilerTools;
		LlvmBuilder builder(context);
		std::unique_ptr<Program> program = std::make_unique<Program>(std::make_unique<llvm::Module>("test", context));
		/*
		program->getModule().setTargetTriple(llvm::sys::getDefaultTargetTriple());

		std::unordered_set<const Noder::Node*> calculatedNodes;
		std::unordered_set<const Noder::Node*> visitedNodes;
		std::unordered_map<const Noder::Node*, std::vector<Value>> nodeOutputs;

		LlvmBuilder::Function function("fun", LlvmBuilder::Type::get<void()>(context), *program);
		LlvmBuilder::InstructionBlock entryPoint(function, "entry");

		builder.setInsertPoint(entryPoint);

		for (int i=0; i<=0; ++i)
		{
			std::vector<const Noder::Node*> nodesToCalculate;
			nodesToCalculate.push_back(&entry);
			visitedNodes.insert(&entry);

			while (!nodesToCalculate.empty())
			{
				const Noder::Node* n = nodesToCalculate.back();
				bool depsAlreadyResolved = true;

				std::unordered_set<const Noder::Node*> neededNodes;

				const auto base = n->getBase();

				assert(base != nullptr);

				for (int i=0; i < base->inputs.size(); ++i)
				{
					auto port = n->getInputPort(i);
					if (!port.isVoid())
					{
						const Noder::Node& t = port.getNode();
						neededNodes.insert(&t);
					}
				}

				for (auto& dep : neededNodes)
				{
					if (!calculatedNodes.count(dep))
					{
						if (visitedNodes.count(dep) > 0)
							throw std::logic_error("Cyclic dependency detected");
						depsAlreadyResolved = false;
						nodesToCalculate.push_back(dep);
						visitedNodes.insert(dep);
					}
				}
				if (depsAlreadyResolved)
				{
					nodesToCalculate.pop_back();
					std::vector<Value> inputs, outputs;
					inputs.resize(base->inputs.size());

					//TODO: adapt to new generators concepts

					//gather inputs
					for (int i = 0; i < inputs.size(); ++i)
					{
						//
					}

					auto nodeStateIt = states.find(n);

					
					for (auto& mapping : n->inputToChildOutputMapping)
					{
						if (mapping.first >= inputs.size())
							throw std::logic_error("There are missing input values");
						inputs[mapping.first] = nodeOutputs.at(mapping.second.first.get())[mapping.second.second];
						n->generate(builder, inputs, outputs);
						nodeOutputs.emplace(n,outputs);
					}
					
					calculatedNodes.insert(n);
				}
			}
		}

		//LlvmBuilder::Value str = builder.createCString("Hello world!\n");

		//LlvmBuilder::ExternalFunction putsFunc("puts", LlvmBuilder::Type::get<int32_t(int8_t*)>(context), *program);
		//builder.addFunctionCall(putsFunc, { str });

		builder.addVoidReturn();
		*/
		return program;
	}
}
