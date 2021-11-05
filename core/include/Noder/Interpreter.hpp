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
			std::function<std::tuple<Rets...>(Args...)> function;
		public:
			virtual void calculate(const std::vector<const State*>& inputs, std::vector<std::unique_ptr<State>>& outputs) override
			{
				std::tuple<Rets...> t(UnpackCaller<std::tuple<Rets...>(Args...)>::call(function, inputs, _noder_hacks_::index_sequence<sizeof...(Args)>()));
				unpackTuple<sizeof...(Rets)>(outputs, t);
			}

			FunctionNodeState(const std::function<std::tuple<Rets...>(Args...)>& f) : function(f) {}
			FunctionNodeState(FunctionNodeState&&) noexcept = default;
		};
		template<typename... Args> class FunctionNodeState<void(Args...)> : public NodeState
		{
		private:
			std::function<void(Args...)> function;
		public:
			virtual void calculate(const std::vector<const State*>& inputs, std::vector<std::unique_ptr<State>>&) override
			{
				UnpackCaller<void(Args...)>::call(function, inputs, _noder_hacks_::index_sequence<sizeof...(Args)>());
			}

			FunctionNodeState(const std::function<void(Args...)>& f) : function(f) {}
			FunctionNodeState(FunctionNodeState&&) noexcept = default;
		};
		template<typename Ret, typename... Args> class FunctionNodeState<Ret(Args...)> : public NodeState
		{
		private:
			std::function<Ret(Args...)> function;
		public:
			virtual void calculate(const std::vector<const State*>& inputs, std::vector<std::unique_ptr<State>>& outputs) override
			{
				outputs.back()->setValue<Ret>(UnpackCaller<Ret(Args...)>::call(function, inputs, _noder_hacks_::index_sequence<sizeof...(Args)>()));
			}

			FunctionNodeState(const std::function<Ret(Args...)>& f) : function(f) {}
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
				std::tuple<Rets...> t(UnpackCaller<std::tuple<Rets...>(Args...)>::call(this, &ObjectNodeState<std::tuple<Rets...>(Args...)>::calculateWrapper, inputs, _noder_hacks_::index_sequence<sizeof...(Args)>()));
				unpackTuple<sizeof...(Rets)>(outputs, t);
			}
			static void preparePorts(NodeTemplate& t)
			{
				unpackTypes<0, Rets...>(t.outputs);
				unpackTypes<0, Args...>(t.inputs);
			}

			virtual std::tuple<Rets...> calculate(Args...) = 0;

			using NodeState::NodeState;
		private:
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
				UnpackCaller<void(Args...)>::call(this, &ObjectNodeState<void(Args...)>::calculateWrapper, inputs, _noder_hacks_::index_sequence<sizeof...(Args)>());
			}

			static void preparePorts(NodeTemplate& t)
			{
				unpackTypes<0, Args...>(t.inputs);
			}

			virtual void calculate(Args...) = 0;

			using NodeState::NodeState;
		private:
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
				outputs.back()->setValue<Ret>(UnpackCaller<Ret(Args...)>::call(this, &ObjectNodeState<Ret(Args...)>::calculateWrapper, inputs, _noder_hacks_::index_sequence<sizeof...(Args)>()));
			}

			virtual Ret calculate(Args...) = 0;

			static void preparePorts(NodeTemplate& t)
			{
				unpackTypes<0, Ret>(t.outputs);
				unpackTypes<0, Args...>(t.inputs);
			}

			using NodeState::NodeState;
		private:
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

		template<unsigned> static void unpackTypes(std::vector<Port>&) {}
		template<unsigned tmp, typename T, typename... Args> static void unpackTypes(std::vector<Port>& p)
		{
			p.emplace_back();
			p.back().setType<T>();
			unpackTypes<tmp, Args...>(p);
		}

		template<unsigned i, typename... Args> static typename std::enable_if<i != 0>::type unpackTuple(std::vector<std::unique_ptr<State>>& outputs, const std::tuple<Args...>& t)
		{
			outputs[sizeof...(Args) - i]->setValue(std::get<sizeof...(Args) - i>(t));
			unpackTuple<i - 1, Args...>(outputs, t);
		}

		template<unsigned i, typename... Args> static typename std::enable_if<i == 0>::type unpackTuple(std::vector<std::unique_ptr<State>>& outputs, const std::tuple<Args...>& t) {}

		template<typename T, size_t I>static T& unpackArgument(const std::vector<const State*>& args)
		{
			return const_cast<State*>(args[I])->getValue<T>();
		}

		template<typename T> struct UnpackCaller {};
		template<typename... Rets, typename... Args> struct UnpackCaller<std::tuple<Rets...>(Args...)>
		{
		public:
			template<size_t... I> static std::tuple<Rets...> call(
				const std::function<std::tuple<Rets...>(Args...)>& f,
				const std::vector<const State*>& inputs,
				_noder_hacks_::sequence<I...> i)
			{
				return f(unpackArgument<Args, I>(inputs)...);
			}
			template<typename T,size_t... I> static std::tuple<Rets...> call(
				T* obj,
				std::tuple<Rets...>(T::* func)(Args...),
				const std::vector<const State*>& inputs,
				_noder_hacks_::sequence<I...> i)
			{
				return (obj->*func)(unpackArgument<Args, I>(inputs)...);
			}
		};

		template<typename... Args> struct UnpackCaller<void(Args...)>
		{
		public:
			template<size_t... I> static void call(
				const std::function<void(Args...)>& f,
				const std::vector<const State*>& inputs,
				_noder_hacks_::sequence<I...> i)
			{
				f(unpackArgument<Args, I>(inputs)...);
			}
			template<typename T, size_t... I> static void call(
				T* obj,
				void(T::* func)(Args...),
				const std::vector<const State*>& inputs,
				_noder_hacks_::sequence<I...> i)
			{
				(obj->*func)(unpackArgument<Args, I>(inputs)...);
			}
		};

		template<typename Ret, typename... Args> struct UnpackCaller<Ret(Args...)>
		{
		public:
			template<size_t... I> static Ret call(
				const std::function<Ret(Args...)>& f,
				const std::vector<const State*>& inputs,
				_noder_hacks_::sequence<I...> i)
			{
				return f(unpackArgument<Args, I>(inputs)...);
			}
			template<typename T, size_t... I> static Ret call(
				T* obj,
				Ret(T::* func)(Args...),
				const std::vector<const State*>& inputs,
				_noder_hacks_::sequence<I...> i)
			{
				return (obj->*func)(unpackArgument<Args, I>(inputs)...);
			}
		};

		template<typename T, typename... Rets, typename... Args> std::function<std::tuple<Rets...>(Args...)>static functorWrapper(const T* obj, std::tuple<Rets...>(T::* func)(Args...) const)
		{
			return std::function<std::tuple<Rets...>(Args...)>(*obj);
		}

		template<typename T, typename R, typename... Args> std::function<R(Args...)> static functorWrapper(const T* obj, R(T::* func)(Args...) const)
		{
			return std::function<R(Args...)>(*obj);
		}

		template<typename T, typename... Args> std::function<void(Args...)> static functorWrapper(const T* obj, void(T::* func)(Args...) const)
		{
			return std::function<void(Args...)>(*obj);
		}
		
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
		NodeTemplate::Ptr createTemplate();

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
			return createTemplate(functorWrapper(&f, &T::operator()), name);
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

