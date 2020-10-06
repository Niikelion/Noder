#pragma once

#include <Noder/Nodes.hpp>
//TODO: wrap llvm to eliminate direct dependency
#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

namespace Noder
{
    class DLLACTION NodeCompiler
    {
    public:
		class ExecutionEngine
		{
		private:
			template<typename T>struct Caller
			{
				using Signature = void*;
			};
		public:
			void* getSymbol(const std::string& name);

			template<typename T> typename Caller<T>::Signature getSymbol(const std::string& name)
			{
				return (Caller<T>::Signature)getSymbol(name);
			}

			ExecutionEngine(const llvm::Module& p);
			ExecutionEngine(ExecutionEngine&&) noexcept = default;
			ExecutionEngine(const ExecutionEngine&) = delete;
		public:
			std::unique_ptr<llvm::ExecutionEngine> engine;
			template<typename ...Args> struct Caller<void(Args...)>
			{
				using Signature = void(*)(Args...);
				void call(void* f, Args... args)
				{
					((Signature)f)(std::forward<Args>(args)...);
				}
			};
			template<typename Ret, typename ...Args> struct Caller<Ret(Args...)>
			{
				using Signature = Ret(*)(Args...);
				Ret call(void* f, Args... args)
				{
					return ((Signature)f)(std::forward<Args>(args)...);
				}
			};
		};

		class Program
		{
		public:
			inline std::shared_ptr<ExecutionEngine> getExecutableInstance()
			{
				if (!engine)
					engine = std::make_shared<ExecutionEngine>(*module);
				return engine;
			}
			void compile(const std::string& filename);

			Program(std::unique_ptr<llvm::Module>&& m) : module(std::move(m)) {}
			Program(Program&&) noexcept = default;
			Program(const Program&) = delete;
		private:
			std::unique_ptr<llvm::Module> module;
			std::shared_ptr<ExecutionEngine> engine;
		};

		class Builder
		{
		public:
		};

		class BuilderModule
		{
		public:
		};

		std::unique_ptr<Program> generate();

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
		llvm::LLVMContext context;
    };
}