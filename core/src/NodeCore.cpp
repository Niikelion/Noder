#include "noder/NodeCore.hpp"

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
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/MCJIT.h"

#include <memory>
#include <iostream>

struct impl_b: public NodeCore::impl
{
	virtual void run()
	{
		using namespace llvm;
		
		InitializeNativeTarget();
		InitializeNativeTargetAsmPrinter();
		
		LLVMContext context;
		IRBuilder<> builder(context);

		std::unique_ptr<Module> module = std::make_unique<Module>("test",context);

		FunctionType* ft = FunctionType::get(Type::getVoidTy(context), {}, false);

		Function* f = Function::Create(ft,Function::ExternalLinkage,"main",module.get());
		BasicBlock* bb = BasicBlock::Create(context,"entry",f);

		builder.SetInsertPoint(bb);

		Value* str = builder.CreateGlobalStringPtr("Hello world!\n");

		std::vector<Type*> putsArgs;
		putsArgs.push_back(builder.getInt8Ty()->getPointerTo());
		
		ArrayRef<llvm::Type*>  argsRef(putsArgs);

		FunctionType* putsType =
			FunctionType::get(builder.getInt32Ty(), argsRef, false);
		llvm::FunctionCallee putsFunc = module->getOrInsertFunction("puts", putsType);

		builder.CreateCall(putsFunc,str);
		builder.CreateRetVoid();
		
		std::string errStr;
		ExecutionEngine* ee = EngineBuilder(std::move(module)).setErrorStr(&errStr).create();

		if (!ee)
		{
			std::cout << "err:" << errStr << std::endl;
		}
		else
		{
			ee->runFunction(f, {});
		}
	}
};

NodeCore::NodeCore()
{
	pImpl = std::make_unique<impl_b>();
	pImpl->run();
}