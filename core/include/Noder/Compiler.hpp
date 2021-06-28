#pragma once

#include <Noder/Nodes.hpp>
//TODO: wrap llvm to eliminate direct dependency
#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

namespace Noder
{
	namespace CompilerTools
	{
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

			inline llvm::Module& getModule()
			{
				return *module;
			}
		private:
			std::unique_ptr<llvm::Module> module;
			std::shared_ptr<ExecutionEngine> engine;
		};

		class LlvmBuilder
		{
		public:
			class Type
			{
			public:
				llvm::Type* type;

				Type pointer()
				{
					return Type(type->getPointerTo());
				}

				Type(llvm::Type* t) : type(t) {}

				template<typename T> static Type get(llvm::LLVMContext& context)
				{
					return Type(TypeTranslator<T>().get(context));
				}
			protected:
				template<typename T, typename = void> struct TypeTranslator
				{
					llvm::Type* get(llvm::LLVMContext& context)
					{
						return nullptr;
					}
				};
				template<typename T> struct TypeTranslator<T, std::enable_if_t<std::is_pointer<T>::value>>
				{
					llvm::Type* get(llvm::LLVMContext& context)
					{
						return TypeTranslator<std::remove_pointer<T>::type>().get(context)->getPointerTo();
					}
				};
				template<typename Ret, typename ...Args> struct TypeTranslator<Ret(Args...)>
				{
					llvm::Type* get(llvm::LLVMContext& context)
					{
						return (llvm::Type*)llvm::FunctionType::get(TypeTranslator<Ret>().get(context), { TypeTranslator<Args>().get(context)... }, false);
					}
				};
				template<> struct TypeTranslator<void>
				{
					llvm::Type* get(llvm::LLVMContext& context)
					{
						return llvm::Type::getVoidTy(context);
					}
				};
				template<typename T> struct TypeTranslator<T, std::enable_if_t<std::is_floating_point<T>::value || std::is_integral<T>::value>>
				{
					llvm::Type* get(llvm::LLVMContext& context)
					{
						return llvm::Type::getScalarTy<T>(context);
					}
				};
			};

			class Value
			{
			public:
				llvm::Value* value;

				Value(llvm::Value* v) : value(v) {}
			};

			class Callee
			{
			public:

				virtual llvm::FunctionCallee getHandle() const = 0;

				inline llvm::Function::LinkageTypes getLinkageType()
				{
					return linkageType;
				}
				inline void setLinkageType(llvm::Function::LinkageTypes linkage)
				{
					linkageType = linkage;
				}
				inline std::string getName() const
				{
					return name;
				}
				inline void setName(const std::string& name)
				{
					this->name = name;
				}

				Callee() : linkageType(llvm::Function::LinkageTypes::CommonLinkage) {}
				Callee(const std::string& n, llvm::Function::LinkageTypes linkage) : linkageType(linkage), name(n) {}
			private:
				llvm::Function::LinkageTypes linkageType;
				std::string name;
			};

			class ExternalFunction : public Callee
			{
			public:
				virtual llvm::FunctionCallee getHandle() const override
				{
					return callee;
				}

				ExternalFunction(const std::string& name, Type signature, Program& program) : Callee(name, llvm::Function::LinkageTypes::ExternalLinkage)
				{
					callee = program.getModule().getOrInsertFunction(name, (llvm::FunctionType*)signature.type);
				}
			private:
				llvm::FunctionCallee callee;
			};
			class Function : public Callee
			{
			public:
				virtual llvm::FunctionCallee getHandle() const override
				{
					return llvm::FunctionCallee(function);
				}


				Function(const std::string& name, Type signature, Program& program, llvm::Function::LinkageTypes linkage = llvm::Function::LinkageTypes::ExternalLinkage) : Callee(name, linkage), function(nullptr)
				{
					function = llvm::Function::Create((llvm::FunctionType*)(signature.type), getLinkageType(), getName(), program.getModule());
				}

				llvm::Function* getFunction()
				{
					return function;
				}
			private:
				llvm::Function* function;
			};

			class InstructionBlock
			{
			public:
				inline llvm::BasicBlock* getBlock()
				{
					return block;
				}

				InstructionBlock(Function& function, const std::string& name = "") : block(nullptr)
				{
					block = llvm::BasicBlock::Create(function.getFunction()->getContext(), name, function.getFunction());
				}
				InstructionBlock(Function& function, const InstructionBlock& insertAnchor, const std::string& name = "") : block(nullptr)
				{
					block = llvm::BasicBlock::Create(function.getFunction()->getContext(), name, function.getFunction(), insertAnchor.block);
				}
			private:
				llvm::BasicBlock* block;
			};

			Value createCString(const std::string& value);

			void addFunctionCall(const Callee& function, const std::vector<Value>& arguments);

			void addReturn(const Value& value);
			void addVoidReturn();

			void setInsertPoint(InstructionBlock& block);

			LlvmBuilder(llvm::LLVMContext& context) : builder(context) {}

			inline llvm::IRBuilder<>& getBuilder()
			{
				return builder;
			}
		private:
			llvm::IRBuilder<> builder;
		};


		class BuilderModule
		{
		public:
			//virtual void updateConfig() {};

			BuilderModule(Node& n) : node(&n) {}
		private:
			Node* node;
		};

		class Value
		{
			//
		};

		class Node
		{
		public:
			std::function<void(Node&, LlvmBuilder& builder,std::vector<Value> inputs,std::vector<Value>& outputs)> generator;
			inline void generate(LlvmBuilder& builder, std::vector<Value> inputs, std::vector<Value>& outputs)
			{
				if (generator)
					generator(*this, builder, inputs, outputs);
			}

			std::vector<Node*> flowOutputs;
			std::map<unsigned, std::pair<std::shared_ptr<Node>, unsigned>> inputToChildOutputMapping;
		};

		class StructureBuilder
		{
		public:
			std::vector<std::shared_ptr<Node>> flowNodes;

			std::unique_ptr<Program> generate(llvm::LLVMContext& context);
		};
	}

    class DLLACTION NodeCompiler
    {
    public:
		std::unique_ptr<CompilerTools::Program> generate();

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