#pragma once

#include <Noder/Nodes.hpp>
//TODO: wrap llvm to eliminate direct dependency
#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>

#include <unordered_set>
#include <unordered_map>

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
				llvm::Value* getValue() const
				{
					return value;
				}

				Value(llvm::Value* v) : value(v) {}
				Value(const Value& v) : value(v.value) {}
				Value() = default;//for some reason you cannot copy vector without it???
			private:
				llvm::Value* value;
			};

			class Callee
			{
			public:
				virtual llvm::FunctionCallee getHandle() const = 0;

				llvm::Function::LinkageTypes getLinkageType() const
				{
					return linkageType;
				}
				void setLinkageType(llvm::Function::LinkageTypes linkage)
				{
					linkageType = linkage;
				}
				std::string getName() const
				{
					return name;
				}
				void setName(const std::string& name)
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
				ExternalFunction(const std::string& name, Type signature, llvm::Module& mod) : Callee(name, llvm::Function::LinkageTypes::ExternalLinkage)
				{
					callee = mod.getOrInsertFunction(name, (llvm::FunctionType*)signature.type);
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

				llvm::Function* getFunction() const
				{
					return function;
				}
			private:
				llvm::Function* function;
			};

			class InstructionBlock
			{
			public:
				llvm::BasicBlock* getBlock() const
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
				InstructionBlock(const InstructionBlock& b): block(b.block) {}
			private:
				llvm::BasicBlock* block;
			};

			class Variable
			{
			public:
				llvm::AllocaInst* getAlloca() const
				{
					return alloca;
				}

				Variable(const InstructionBlock& block, const LlvmBuilder::Type& type): alloca(nullptr)
				{
					llvm::IRBuilder<> b(block.getBlock());
					alloca = b.CreateAlloca(type.type);
				}
			private:
				llvm::AllocaInst* alloca;
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

		template<typename T, typename = void> struct TypeTranslator {};
		template<typename T> struct TypeTranslator<T, std::enable_if_t<std::is_integral_v<T>>>
		{
			static LlvmBuilder::Value translate(T value, llvm::LLVMContext& context)
			{
				return LlvmBuilder::Value(llvm::ConstantInt::get(LlvmBuilder::Type::get<T>(context), static_cast<uint64_t>(value), std::is_signed_v<T>));
			}
		};
		template<typename T> struct TypeTranslator<T, std::enable_if_t<std::is_floating_point_v<T>>>
		{
			static LlvmBuilder::Value translate(T value, llvm::LLVMContext& context)
			{
				return LlvmBuilder::Value(llvm::ConstantFP::get(LlvmBuilder::Type::get<T>(context), static_cast<double>(value)));
			}
		};
		using TranslationMapping = std::unordered_map<std::type_index, std::function<LlvmBuilder::Value(const PortState * s, llvm::LLVMContext&)>>;
		template<typename... Args> struct TranslationsGenerator
		{
			static void addTranslation(TranslationMapping& trans) {}
		};
		template<typename T, typename... Args> struct TranslationsGenerator<T, Args...>
		{
			static void addTranslation(TranslationMapping& trans)
			{
				trans.emplace(typeid(T), [](const PortState* state, llvm::LLVMContext& context)
					{
						CompilerTools::TypeTranslator<T>::translate(state->getValue<T>(), context);
					});
				TranslationsGenerator<Args...>::addTranslation(trans);
			}
		};
	}

    class DLLACTION NodeCompiler
    {
    public:
		class NodeState
		{
		public:
			virtual void generate(
				const Node& entry,
				CompilerTools::LlvmBuilder& builder,
				CompilerTools::LlvmBuilder::Function function,
				std::vector<CompilerTools::LlvmBuilder::InstructionBlock> flowInputs,
				std::vector<CompilerTools::LlvmBuilder::InstructionBlock>& flowOutputs,
				std::vector<CompilerTools::LlvmBuilder::Value> inputs,
				std::vector<CompilerTools::LlvmBuilder::Value>& outputs) = 0;

			virtual CompilerTools::LlvmBuilder::Value translate(const PortState* state, llvm::LLVMContext& context) = 0;
		};

		template<typename T> class ObjectNodeState : public NodeState
		{
		public:
			using ReturnTypes = typename TypeUtils::SignatureSplitter<T>::ReturnTypes;
			using ArgumentTypes = typename TypeUtils::SignatureSplitter<T>::ArgumentTypes;

			virtual CompilerTools::LlvmBuilder::Value translate(const PortState* state, llvm::LLVMContext& context) override
			{
				auto transIt = translations.find(state->getType());
				if (transIt == translations.end())
				{
					//TODO: throw, this should not happen
				}
				return transIt->second(state, context);
			}
			ObjectNodeState()
			{
				generateTranslations(translations, ArgumentTypes{});
			}
		private:
			CompilerTools::TranslationMapping translations;

			template<typename... Args> static void generateTranslations(CompilerTools::TranslationMapping& trans, TypeUtils::Types<Args...>)
			{
				CompilerTools::TranslationsGenerator<Args...>::addTranslation(trans);
			}
		};

		//TODO: extend generator type and allow access to state variable
		using GeneratorType = void(
			const Node& entry,
			CompilerTools::LlvmBuilder& builder,
			CompilerTools::LlvmBuilder::Function function,
			std::vector<CompilerTools::LlvmBuilder::InstructionBlock> flowInputs,
			std::vector<CompilerTools::LlvmBuilder::InstructionBlock>& flowOutputs,
			std::vector<CompilerTools::LlvmBuilder::Value> inputs,
			std::vector<CompilerTools::LlvmBuilder::Value>& outputs);

		std::unique_ptr<CompilerTools::Program> generate(const Node& mainEntry);
		std::unique_ptr<CompilerTools::Program> generateFunction(const Node& mainEntry, const std::string& name);

		static void initializeLlvm();

		std::unique_ptr<Enviroment> extractEnviroment();
		std::unique_ptr<Enviroment> swapEnviroment(std::unique_ptr<Enviroment>&&);
		Enviroment& getEnviroment();

		Node::Ptr createNode(const NodeTemplate::Ptr& base);
		NodeTemplate::Ptr createTemplate(const std::string& name, const std::vector<Port>& inP, const std::vector<Port>& outP, unsigned flowInP, unsigned flowOutP, const std::function<std::unique_ptr<NodeState>(const Node&)>& factory);

		template<typename... Rets, typename... Args> NodeTemplate::Ptr createTemplate(const std::string& name, TypeUtils::Types<Rets...>, TypeUtils::Types<Args...>, unsigned flowInP, unsigned flowOutP, const std::function<std::unique_ptr<NodeState>(const Node&)>& factory)
		{
			std::vector<Port> inputs, outputs;
			TypeUtils::unpackPorts<Args...>(inputs);
			TypeUtils::unpackPorts<Rets...>(outputs);
			return createTemplate(name, inputs, outputs, flowInP, flowOutP, factory);
		}

		template<typename T> NodeTemplate::Ptr createTemplate(const std::string& name="")
		{
			return createTemplate(name, T::ReturnTypes{}, T::ArgumentTypes{}, 0, 0, [](const Node& n) {
				return std::make_unique<T>(n, state);
			});
		}

		template<typename T> NodeTemplate::Ptr createTemplate(const std::function<GeneratorType>& generator, const std::string& name = "", unsigned flowInP = 0, unsigned flowOutP = 0)
		{
			using Signature = TypeUtils::SignatureSplitter<T>;
			return createTemplate(name, Signature::ReturnTypes{}, Signature::ArgumentTypes{}, flowInP, flowOutP, [generator](const Node&)
				{
					return std::make_unique<FunctionNodeState<T>>(generator);
				});
		}

		void resetEnviroment();

		void resetFactories();

		NodeCompiler();
		NodeCompiler(std::unique_ptr<Enviroment>&&);
    private:
		template<typename T> class FunctionNodeState : public NodeState
		{
		public:
			virtual void generate(
				const Node& entry,
				CompilerTools::LlvmBuilder& builder,
				CompilerTools::LlvmBuilder::Function function,
				std::vector<CompilerTools::LlvmBuilder::InstructionBlock> flowInputs,
				std::vector<CompilerTools::LlvmBuilder::InstructionBlock>& flowOutputs,
				std::vector<CompilerTools::LlvmBuilder::Value> inputs,
				std::vector<CompilerTools::LlvmBuilder::Value>& outputs) override
			{
				if (!generator)
				{
					//TODO: throw, missing generator function
				}
				generator(entry, builder, function, flowInputs, flowOutputs, inputs, outputs);
			}

			virtual CompilerTools::LlvmBuilder::Value translate(const PortState* state, llvm::LLVMContext& context) override
			{
				auto transIt = translations.find(state->getType());
				if (transIt == translations.end())
				{
					//TODO: throw, this should not happen
				}
				return transIt->second(state, context);
			}

			FunctionNodeState(const std::function<GeneratorType>& gen) : generator(gen)
			{
				generateTranslations(translations, TypeUtils::SignatureSplitter<T>::ArgumentTypes{});
			}
		private:
			template<typename... Args> static void generateTranslations(CompilerTools::TranslationMapping& trans, TypeUtils::Types<Args...>)
			{
				CompilerTools::TranslationsGenerator<Args...>::addTranslation(trans);
			}

			CompilerTools::TranslationMapping translations;
			std::function<GeneratorType> generator;
		};

		class CompilerState
		{
		public:
			CompilerTools::LlvmBuilder& builder;
			std::unordered_set<const Node*> calculatedNodes;
			std::unordered_set<const Node*> visitedNodes;
			std::unordered_map<const Node*, std::vector<CompilerTools::LlvmBuilder::Value>> nodeOutputs;

			CompilerState(CompilerTools::LlvmBuilder& b) : builder(b) {}
		};

		std::unordered_map<std::string, std::function<std::shared_ptr<NodeState>(const Node&)>> stateFactories;
		std::unordered_map<const Node*, std::shared_ptr<NodeState>> states;
        std::unique_ptr<Enviroment> env;
		llvm::LLVMContext context;

		std::string correctName(const std::string& name);

		std::vector<CompilerTools::LlvmBuilder::InstructionBlock> generateNode(
			const Node& node,
			CompilerTools::LlvmBuilder& builder,
			CompilerTools::LlvmBuilder::Function function,
			std::vector<CompilerTools::LlvmBuilder::InstructionBlock> entries,
			std::vector<CompilerTools::LlvmBuilder::Value> inputs,
			std::vector<CompilerTools::LlvmBuilder::Value>& outputs);

		void generateInstruction(
			const Node& entry,
			CompilerState& state,
			CompilerTools::LlvmBuilder::Function function,
			std::vector<CompilerTools::LlvmBuilder::InstructionBlock> flowInputs,
			std::vector<CompilerTools::LlvmBuilder::InstructionBlock>& flowOutputs);

		void generateFull(
			const Node& entry,
			CompilerTools::LlvmBuilder& builder,
			CompilerTools::LlvmBuilder::Function function);
    };
}