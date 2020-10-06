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
//#include ""

#include <memory>
#include <iostream>

namespace Noder
{
	std::unique_ptr<NodeCompiler::Program> NodeCompiler::generate()
	{
		using namespace llvm;
		
		//llvm::CodeGenFileType::CGFT_ObjectFile

		IRBuilder<> builder(context);

		std::unique_ptr<Module> module = std::make_unique<Module>("test", context);
		module->setTargetTriple(llvm::sys::getDefaultTargetTriple());

		FunctionType* ft = FunctionType::get(Type::getVoidTy(context), {}, false);

		Function* f = Function::Create(ft, Function::ExternalLinkage, "printer", module.get());
		BasicBlock* bb = BasicBlock::Create(context, "entry", f);

		builder.SetInsertPoint(bb);

		Value* str = builder.CreateGlobalStringPtr("Hello world!\n");

		std::vector<Type*> putsArgs;
		putsArgs.push_back(builder.getInt8Ty()->getPointerTo());

		ArrayRef<llvm::Type*>  argsRef(putsArgs);

		FunctionType* putsType =
			FunctionType::get(builder.getInt32Ty(), argsRef, false);
		llvm::FunctionCallee putsFunc = module->getOrInsertFunction("puts", putsType);

		builder.CreateCall(putsFunc, str);
		builder.CreateRetVoid();

		return std::make_unique<Program>(std::move(module));
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
		//TODO:
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

	void* NodeCompiler::ExecutionEngine::getSymbol(const std::string& name)
	{
		return engine->getPointerToNamedFunction(name);
	}
	NodeCompiler::ExecutionEngine::ExecutionEngine(const llvm::Module& m)
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
	void NodeCompiler::Program::compile(const std::string& filename)
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
}