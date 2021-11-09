#pragma once

#include <Noder/Nodes.hpp>
#include <unordered_set>
#include <functional>
#include <algorithm>
#include <vector>

namespace Noder
{
	class DLLACTION NodeCycleException : public std::exception
	{
	public:
		virtual const char* what() const noexcept
		{
			return "Unexpected cyclic dependecny detected!";
		}
	};

	class DLLACTION NodeInterpreter
	{
	public:
		class NodeState
		{
		public:
			inline std::vector<std::unique_ptr<State>>& getCachedOutputs()
			{
				return cachedOutputs;
			}
			inline unsigned getTargetFlowPort()
			{
				return targetFlowPort;
			}
			inline bool pushesNewScope()
			{
				return pushesScope;
			}
			template<typename T> void pushScopedValue(const T& value)
			{
				if (!(*scopedVariable))
				{
					*scopedVariable = std::make_unique<State>(_noder_hacks_::tag<T>{});
					(*scopedVariable)->emplaceValue<T>(value);
				}
			}

			template<typename T> T popScopedValue()
			{
				if (scopedVariable && scopedVariable->checkType<T>())
				{
					T ret = (*scopedVariable)->getValue<T>();
					scopedVariable->reset();
					return std::move(ret);
				}
			}

			void resetInternals();

			void softReset();
			void hardReset();

			void setUpInternals(std::unique_ptr<State>& scopeVariable);

			virtual void attachNode(const Node& node);

			virtual void calculate(const std::vector<const State*>& inputs, std::vector<std::unique_ptr<State>>& outputs) = 0;
			virtual void resetState() {};

			NodeState() : targetFlowPort(0), pushesScope(false), scopedVariable(nullptr) {}

			NodeState(NodeState&&) = default;
		protected:
			inline void redirectFlow(unsigned port)
			{
				targetFlowPort = port;
			}
			inline void pushScope()
			{
				pushesScope = true;
			}
		private:
			unsigned targetFlowPort;
			bool pushesScope;
			std::vector<std::unique_ptr<State>> cachedOutputs;
			std::unique_ptr<State>* scopedVariable;
		};

		template<typename T> class FunctionNodeState : public NodeState {};
		template<typename... Rets, typename... Args> class FunctionNodeState<std::tuple<Rets...>(Args...)> : public NodeState
		{
		private:
			using Signature = std::tuple<Rets...>(Args...);
			std::function<Signature> function;
		public:
			virtual void calculate(const std::vector<const State*>& inputs, std::vector<std::unique_ptr<State>>& outputs) override
			{
				TypeUtils::VectorCaller<Signature>::call(function, inputs, outputs, _noder_hacks_::index_sequence<sizeof...(Args)>{});
			}

			FunctionNodeState(const std::function<Signature>& f) : function(f) {}
			FunctionNodeState(FunctionNodeState&&) noexcept = default;
		};
		template<typename... Args> class FunctionNodeState<void(Args...)> : public NodeState
		{
		private:
			using Signature = void(Args...);
			std::function<Signature> function;
		public:
			virtual void calculate(const std::vector<const State*>& inputs, std::vector<std::unique_ptr<State>>& outputs) override
			{
				TypeUtils::VectorCaller<Signature>::call(function, inputs, outputs, _noder_hacks_::index_sequence<sizeof...(Args)>{});
			}

			FunctionNodeState(const std::function<Signature>& f) : function(f) {}
			FunctionNodeState(FunctionNodeState&&) noexcept = default;
		};
		template<typename Ret, typename... Args> class FunctionNodeState<Ret(Args...)> : public NodeState
		{
		private:
			using Signature = Ret(Args...);
			std::function<Signature> function;
		public:
			virtual void calculate(const std::vector<const State*>& inputs, std::vector<std::unique_ptr<State>>& outputs) override
			{
				TypeUtils::VectorCaller<Signature>::call(function, inputs, outputs, _noder_hacks_::index_sequence<sizeof...(Args)>{});
			}

			FunctionNodeState(const std::function<Signature>& f) : function(f) {}
			FunctionNodeState(FunctionNodeState&&) noexcept = default;
		};
	
		template<typename T> class ObjectNodeState : public NodeState {};
		template<typename... Rets, typename... Args> class ObjectNodeState<std::tuple<Rets...>(Args...)> : public NodeState
		{
		public:
			using ArgumentTypes = _noder_hacks_::types<Args...>;
			using ReturnTypes = _noder_hacks_::types<Rets...>;

			virtual void calculate(const std::vector<const State*>& inputs, std::vector<std::unique_ptr<State>>& outputs) override
			{
				TypeUtils::VectorCaller<Signature>::call(this, &ObjectNodeState<Signature>::calculateWrapper, inputs, outputs, _noder_hacks_::index_sequence<sizeof...(Args)>{});
			}

			virtual std::tuple<Rets...> calculate(Args...) = 0;

			using NodeState::NodeState;
		private:
			using Signature = std::tuple<Rets...>(Args...);
			std::tuple<Rets...> calculateWrapper(Args... args)
			{
				return calculate(args...);
			}
		};
		template<typename... Args> class ObjectNodeState<void(Args...)> : public NodeState
		{
		public:
			using ArgumentTypes = _noder_hacks_::types<Args...>;
			using ReturnTypes = _noder_hacks_::types<>;

			virtual void calculate(const std::vector<const State*>& inputs, std::vector<std::unique_ptr<State>>& outputs) override
			{
				TypeUtils::VectorCaller<Signature>::call(this, &ObjectNodeState<Signature>::calculateWrapper, inputs, outputs, _noder_hacks_::index_sequence<sizeof...(Args)>{});
			}

			virtual void calculate(Args...) = 0;

			using NodeState::NodeState;
		private:
			using Signature = void(Args...);
			void calculateWrapper(Args... args)
			{
				calculate(args...);
			}
		};
		template<typename Ret, typename... Args> class ObjectNodeState<Ret(Args...)> : public NodeState
		{
		public:
			using ArgumentTypes = _noder_hacks_::types<Args...>;
			using ReturnTypes = _noder_hacks_::types<Ret>;

			virtual void calculate(const std::vector<const State*>& inputs, std::vector<std::unique_ptr<State>>& outputs) override
			{
				TypeUtils::VectorCaller<Signature>::call(this, &ObjectNodeState<Signature>::calculateWrapper, inputs, outputs, _noder_hacks_::index_sequence<sizeof...(Args)>{});
			}

			virtual Ret calculate(Args...) = 0;

			static void preparePorts(NodeTemplate& t)
			{
				unpackTypes<0, Ret>(t.outputs);
				unpackTypes<0, Args...>(t.inputs);
			}

			using NodeState::NodeState;
		private:
			using Signature = Ret(Args...);
			Ret calculateWrapper(Args... args)
			{
				return calculate(args...);
			}
		};

	private:
		struct DLLACTION Scope
		{
			const Node* origin;
			std::unordered_set<const Node*> nodesToReset;

			Scope(const Node* n) : origin(n) {}
			Scope(const Scope&) = default;
		};

		std::vector<Scope> pushedScopes;
		std::unordered_set<const Node*> nodesToReset;
		std::unordered_map<const Node*, std::unique_ptr<NodeState>> states;
		std::unique_ptr<Enviroment> env;
		std::unordered_map<std::string, std::function<std::unique_ptr<NodeState>(const Node&, std::unique_ptr<State>&)>> stateFactories;
		std::unique_ptr<State> scopedVariable;

		void pushScope(const Node& n);
		const Node* popScope();

		void calcNode(const Node& endPoint, std::unordered_set<const Node*>& visited, std::unordered_set<const Node*>& toReset);
		
		std::string correctName(const std::string& name);

		template<typename T> struct TypeUnpacker {};
		template<typename... Rets, typename... Args> struct TypeUnpacker<std::tuple<Rets...>(Args...)>
		{
		public:
			static void unpackOutputs(std::vector<Port>& outputs)
			{
				unpackTypes<0, Rets...>(outputs);
			}
			static void unpackInputs(std::vector<Port>& inputs)
			{
				unpackTypes<0, Args...>(inputs);
			}
		};

		template<typename T> static std::unique_ptr<NodeState> genericStateCreatorFunction(const Node& node, std::unique_ptr<State>& i)
		{
			auto ret = std::make_unique<T>();
			ret->setUpInternals(i);
			ret->attachNode(node);
			return ret;
		}
	public:
		void buildState(Node& node);
		void rebuildState(Node& node);
		void buildStates();

		Node::Ptr createNode(const NodeTemplate::Ptr&);

		NodeTemplate::Ptr createTemplate(const std::string& name, const std::vector<Port>& inP, const std::vector<Port>& outP, unsigned flowInP, unsigned flowOutP, const std::function<std::unique_ptr<NodeState>(const Node&,std::unique_ptr<State>&)>& factory);

		template<typename... Rets, typename... Args> NodeTemplate::Ptr createTemplate(const std::string& name, _noder_hacks_::types<Args...>, _noder_hacks_::types<Rets...>, const std::function<std::unique_ptr<NodeState>(const Node&, std::unique_ptr<State>&)>& factory, unsigned flowInP = 0, unsigned flowOutP = 0)
		{
			std::vector<Port> inputs, outputs;
			TypeUtils::unpackPorts<Args...>(inputs);
			TypeUtils::unpackPorts<Rets...>(outputs);
			return createTemplate(name, inputs, outputs, flowInP, flowOutP, factory);
		}

		template<typename T> NodeTemplate::Ptr createTemplate(const std::string& name="")
		{
			return createTemplate(name, T::ArgumentTypes{}, T::ReturnTypes{}, genericStateCreatorFunction<T>, 0, 0);
		}

		template<typename... Rets, typename... Args> NodeTemplate::Ptr createTemplate(const std::function<std::tuple<Rets...>(Args...)>& f, const std::string& name = "")
		{
			return createTemplate(
				name,
				_noder_hacks_::types<Args...>{},
				_noder_hacks_::types<Rets...>{},
				[f](const Node& node, std::unique_ptr<State>& i) -> std::unique_ptr<NodeState>
				{
					auto ret = std::make_unique<FunctionNodeState<std::tuple<Rets...>(Args...)>>(f);
					ret->setUpInternals(i);
					ret->attachNode(node);
					return ret;
				}, 0, 0);
		}
		template<typename... Rets, typename... Args> NodeTemplate::Ptr createTemplate(std::tuple<Rets...>(*f)(Args...), const std::string& name = "")
		{
			return createTemplate(std::function<std::tuple<Rets...>(Args...)>(f),name);
		}

		template<typename... Args> NodeTemplate::Ptr createTemplate(const std::function<void(Args...)>& f, const std::string& name = "")
		{
			return createTemplate(
				name,
				_noder_hacks_::types<Args...>{},
				_noder_hacks_::types<>{},
				[f](const Node& node, std::unique_ptr<State>& i)
				{
					auto ret = std::make_unique<FunctionNodeState<void(Args...)>>(f);
					ret->setUpInternals(i);
					ret->attachNode(node);
					return ret;
				}, 0, 0);
		}
		template<typename... Args> NodeTemplate::Ptr createTemplate(void(*f)(Args...), const std::string& name = "")
		{
			return createTemplate(std::function<void(Args...)>(f),name);
		}

		template<typename T, typename... Args> NodeTemplate::Ptr createTemplate(const std::function<T(Args...)>& f, const std::string& name = "")
		{
			return createTemplate(
				name,
				_noder_hacks_::types<Args...>{},
				_noder_hacks_::types<T>{},
				[f](const Node& node, std::unique_ptr<State>& i)
				{
					auto ret = std::make_unique<FunctionNodeState<T(Args...)>>(f);
					ret->setUpInternals(i);
					ret->attachNode(node);
					return ret;
				}, 0 ,0);
		}
		template<typename T, typename... Args> NodeTemplate::Ptr createTemplate(T(*f)(Args...), const std::string& name = "")
		{
			return createTemplate(std::function<T(Args...)>(f),name);
		}

		template<typename T, typename R = decltype(&T::operator())> NodeTemplate::Ptr createTemplate(const T& f, const std::string& name = "")
		{
			return createTemplate(TypeUtils::functorWrapper(&f, &T::operator()), name);
		}

		void addStateFactory(const std::string& name ,const std::function<std::unique_ptr<NodeState>(const Node&,std::unique_ptr<State>&)>& factory);

		NodeState* getNodeState(Node&);

		std::unique_ptr<Enviroment> extractEnviroment();
		std::unique_ptr<Enviroment> swapEnviroment(std::unique_ptr<Enviroment>&&);
		Enviroment& getEnviroment();

		void resetEnviroment();
		void resetStates();
		void resetState(const Node& node);
		void softResetState(const Node& node);

		void resetFactories();

		void runFrom(const Node& startPoint);
		void calcNode(const Node& endPoint);

		NodeInterpreter();
		NodeInterpreter(std::unique_ptr<Enviroment>&&);
	};
}

