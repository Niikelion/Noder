#pragma once

#include "Nodes.hpp"
#include <unordered_set>
#include <functional>
#include <algorithm>
#include <vector>

namespace Noder
{
	namespace _noder_hacks_
	{
		template<typename T> using Extract = typename T::type;
		template<size_t...> struct sequence { using type = sequence; };
		
		template<typename T1, typename T2> struct combine;
		template<size_t... I1, size_t... I2> struct combine<sequence<I1...>, sequence<I2...>> : sequence<I1..., (I2 + sizeof...(I1))...> {};
		template<typename T1, typename T2> using Combine = Extract<combine<T1, T2>>;

		template<size_t N> struct sequence_generator;
		template<size_t N> using index_sequence = Extract<sequence_generator<N>>;

		template<size_t N> struct sequence_generator : Combine<index_sequence<N / 2>, index_sequence<N - N / 2>> {};
		template<> struct sequence_generator<0> : sequence<> {};
		template<> struct sequence_generator<1> : sequence<0> {};
	}

	class DLLACTION std::exception;

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
		class DLLACTION NodeState
		{
		public:
			class DLLACTION Access
			{
			private:
				Node* node;
				NodeState* state;
			public:
				template<typename T>T& getConfigValue()
				{
					return node->getConfig()->getValue<T>();
				}
				State& getState();

				unsigned getTargetFlowPort() const;
				void redirectFlow(unsigned port);

				void pushScope();

				Access(Node* n, NodeState* s) : node(n), state(s) {}
				Access(const Access&) = default;
			};

			std::vector<std::unique_ptr<State>> outputs;
			std::unique_ptr<State> internal;
			std::vector<std::unique_ptr<State>> internal_inputs;
			unsigned targetFlowPort;
			bool pushesScope;

			void resetInternals();
			void softReset();
			void hardReset();

			NodeState() : outputs(), internal(), internal_inputs(), targetFlowPort(0), pushesScope(false) {}
			NodeState(const NodeState&) = delete;
			NodeState(NodeState&&) noexcept = default;
		};
	private:
		struct DLLACTION Scope
		{
			Node* origin;
			std::unordered_set<Node*> nodesToReset;

			Scope(Node* n) : origin(n) {}
			Scope(const Scope&) = default;
		};

		std::vector<Scope> pushedScopes;
		std::unordered_set<Node*> nodesToReset;
		std::unordered_map<Node*, NodeState> states;
		std::unique_ptr<Enviroment> env;
		std::unordered_map<std::string, std::function<void(NodeState::Access,const std::vector<State*>&, std::vector<std::unique_ptr<State>>&)>> mapping;

		void pushScope(Node* n);
		Node* popScope();

		void calcNode(Node& endPoint, std::unordered_set<Node*>& visited, std::unordered_set<Node*>& toReset);

		template<unsigned> static void unpackTypes(std::vector<Port>&) {}
		template<unsigned tmp,typename T, typename... Args> static void unpackTypes(std::vector<Port>& p)
		{
			p.emplace_back();
			p.back().setType<T>();
			unpackTypes<tmp,Args...>(p);
		}

		template<unsigned i,typename... Args> static typename std::enable_if<i != 0>::type unpackTuple(std::vector<std::unique_ptr<State>>& outputs,const std::tuple<Args...>& t)
		{
			outputs[sizeof...(Args) - i]->setValue(std::get<sizeof...(Args) - i>(t));
			unpackTuple<i - 1, Args...>(outputs,t);
		}

		template<unsigned i,typename... Args> static typename std::enable_if<i == 0>::type unpackTuple(std::vector<std::unique_ptr<State>>& outputs, const std::tuple<Args...>& t) {}
	
		template<typename T,size_t I>static T& unpackArgument(const std::vector<State*>& args)
		{
			return args[I]->getValue<T>();
		}

		template<typename... Args1, typename... Args2, size_t... I> std::tuple<Args1...>static call_with_unpack(
			const std::function<std::tuple<Args1...>(Args2...)>& f,
			const std::vector<State*>& inputs,
			_noder_hacks_::sequence<I...> i)
		{
			return (f(unpackArgument<Args2, I>(inputs)...));
		}

		template<typename... Args1, typename... Args2, size_t... I> std::tuple<Args1...>static call_with_unpack_and_states(
			const std::function<std::tuple<Args1...>(NodeState::Access, Args2...)>& f,
			NodeState::Access access,
			const std::vector<State*>& inputs,
			_noder_hacks_::sequence<I...> i)
		{
			return (f(access,unpackArgument<Args2, I>(inputs)...));
		}

		template<typename T, typename ...Args1, typename ...Args2> std::function<std::tuple<Args1...>(Args2...)>static functorWrapper(T* obj, std::tuple<Args1...>(T::* func)(Args2...) const)
		{
			return [func, obj](Args2&&... args) -> std::tuple<Args1...>
			{
				return ((*obj).*(func))(std::forward<Args2>(args)...);
			};
		}

		template<typename T,typename R, typename ...Args> std::function<std::tuple<R>(Args...)>static functorWrapper(T* obj, R(T::* func)(Args...) const)
		{
			return [func, obj](Args&&... args) -> std::tuple<R>
			{
				return ((*obj).*(func))(std::forward<Args>(args)...);
			};
		}

		template<typename T, typename ...Args> std::function<std::tuple<>(Args...)>static functorWrapper(T* obj, void(T::* func)(Args...) const)
		{
			return [func, obj](Args&&... args) -> std::tuple<>
			{
				((*obj).*(func))(std::forward<Args>(args)...);
				return std::tuple<>();
			};
		}
	public:

		void buildStates();

		Node& createNode(NodeTemplate&);
		NodeTemplate& createTemplate();

		NodeTemplate& createTemplate(const std::string& name,const std::vector<Port>& inP,const std::vector<Port>& outP, const std::function<void(NodeState::Access, const std::vector<State*>&, std::vector<std::unique_ptr<State>>&)>&);

		template<typename... Args1, typename... Args2>NodeTemplate& createTemplate(std::tuple<Args1...>(*f)(Args2...))
		{
			return createTemplate(std::function<std::tuple<Args1...>(Args2...)>(f));
		}

		template<typename T, typename R = decltype(&T::operator())>NodeTemplate& createTemplate(T& f)
		{
			return createTemplate(functorWrapper(&f, &T::operator()));
		}

		template<typename... Args> NodeTemplate& createTemplate(void(*f)(Args...))
		{
			return createTemplate(std::function<std::tuple<>(Args...)>([f](Args... args)
				{
					f(args...);
					return std::tuple<>();
				}));
		}

		template<typename T, typename... Args> NodeTemplate& createTemplate(T(*f)(Args...))
		{
			return createTemplate(std::function<std::tuple<T>(Args...)>([f](Args... args)
				{
					return std::make_tuple(f(args...));
				}));
		}

		template<typename... Args1, typename... Args2>NodeTemplate& createTemplate(const std::function<std::tuple<Args1...>(Args2...)>& f)
		{
			NodeTemplate& t = createTemplate();

			unpackTypes<0,Args1...>(t.outputs);
			unpackTypes<0,Args2...>(t.inputs);

			std::string a = "__generated" + std::to_string(mapping.size());

			setMapping(a, [f](NodeState::Access,const std::vector<State*>& inputs, std::vector<std::unique_ptr<State>>& outputs) 
				{
					std::tuple<Args1...> t((call_with_unpack(f,inputs,_noder_hacks_::index_sequence<sizeof...(Args2)>())));
					unpackTuple<sizeof...(Args1)>(outputs,t);
				});
			
			t.action = a;

			return t;
		}





		template<typename... Args1, typename... Args2>NodeTemplate& createTemplateWithStates(std::tuple<Args1...>(*f)(NodeState::Access,Args2...))
		{
			return createTemplateWithStates(std::function<std::tuple<Args1...>(NodeState::Access,Args2...)>(f));
		}

		template<typename T,typename R = decltype(&T::operator())>NodeTemplate& createTemplateWithStates(T& f)
		{
			return createTemplateWithStates(functorWrapper(&f,&T::operator()));
		}

		template<typename... Args> NodeTemplate& createTemplateWithStates(void(*f)(NodeState::Access, Args...))
		{
			return createTemplateWithStates(std::function<std::tuple<>(NodeState::Access, Args...)>([f](NodeState::Access access, Args... args)
				{
					f(access, args...);
					return std::tuple<>();
				}));
		}

		template<typename T, typename... Args> NodeTemplate& createTemplateWithStates(T(*f)(NodeState::Access, Args...))
		{
			return createTemplateWithStates(std::function<std::tuple<T>(NodeState::Access, Args...)>([f](NodeState::Access access, Args... args)
				{
					return std::make_tuple(f(access, args...));
				}));
		}

		template<typename... Args1, typename... Args2>NodeTemplate& createTemplateWithStates(const std::function<std::tuple<Args1...>(NodeState::Access, Args2...)>& f)
		{
			NodeTemplate& t = createTemplate();

			unpackTypes<0, Args1...>(t.outputs);
			unpackTypes<0, Args2...>(t.inputs);

			std::string a = "__generated" + std::to_string(mapping.size());

			setMapping(a, [f](NodeState::Access access, const std::vector<State*>& inputs, std::vector<std::unique_ptr<State>>& outputs)
				{
					std::tuple<Args1...> t((call_with_unpack_and_states(f, access, inputs, _noder_hacks_::index_sequence<sizeof...(Args2)>())));
					unpackTuple<sizeof...(Args1)>(outputs, t);
				});

			t.action = a;

			return t;
		}

		void setMapping(const std::string& s, const std::function<void(NodeState::Access,const std::vector<State*>&, std::vector<std::unique_ptr<State>>&)>& f);

		NodeState* getNodeState(Node&);

		std::unique_ptr<Enviroment> extractEnviroment();
		std::unique_ptr<Enviroment> swapEnviroment(std::unique_ptr<Enviroment>&&);
		void resetEnviroment();
		void resetStates();
		void resetState(Node* node);
		void softResetState(Node* node);

		void runFrom(Node& startPoint);
		void calcNode(Node& endPoint);

		NodeInterpreter();
		NodeInterpreter(std::unique_ptr<Enviroment>&&);
	};
}

