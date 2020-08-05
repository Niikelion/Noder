#pragma once

#include <vector>
#include <typeindex>
#include <exception>
#include <string>
#include <memory>
#include <unordered_map>

#include "nodeutils.hpp"

namespace Noder
{
	class DLLACTION TypeMismatchException : public std::exception
	{
	private:
		std::string msg;
	public:
		virtual const char* what() const noexcept;
		TypeMismatchException(const std::type_index& source, const std::type_index& target) noexcept;
	};

	class DLLACTION State
	{
	private:
		std::vector<char> data;
		void* pointer;
		std::type_index info;
		void(*deleter)(void*);
		template<typename T> static void deleterFunc(void* p)
		{
			reinterpret_cast<T*>(p)->~T();
		}
	public:
		template<typename T> void setValue(const T& value)
		{
			if (checkType<T>())
			{
				if (pointer == nullptr)
				{
					pointer = new(data.data()) T(value);
					deleter = deleterFunc<T>;
				}
				else
					*reinterpret_cast<T*>(pointer) = value;
			}
		}

		template<typename T, typename ...Args> void emplaceValue(Args... args)
		{
			if (pointer == nullptr && checkType<T>())
			{
				pointer = new(data.data()) T(std::forward(args)...);
				deleter = deleterFunc<T>;
			}
		}

		template<typename T> bool checkType() const noexcept
		{
			return info == typeid(T);
		}

		bool isReady() const noexcept;
		void reset();

		template<typename T> T& getValue()
		{
			if (checkType<T>())
			{
				return *reinterpret_cast<T*>(pointer);
			}
			else
			{
				throw TypeMismatchException(info,typeid(T));
			}
		}

		template<typename T> const T& getValue() const
		{
			if (checkType<T>())
			{
				return *reinterpret_cast<T*>(pointer);
			}
			else
			{
				throw TypeMismatchException(info, typeid(T));
			}
		}

		template<typename T> State() : pointer(nullptr),deleter(nullptr), info(typeid(T))
		{
			data.resize(sizeof(T));
		}
		State(const std::type_index& type, size_t size) : pointer(nullptr), deleter(nullptr), info(type)
		{
			data.resize(size);
		}
		State(const State&) = delete;
		State(State&&) noexcept = default;
		~State();
	};

	using PortState = State;

	class DLLACTION Port
	{
	private:
		template<typename T> static std::unique_ptr<State> pointStateFactory()
		{
			return std::make_unique<State>(typeid(T),sizeof(T));
		}
		std::type_index type;
		std::unique_ptr<State>(*factory)();
	public:
		std::type_index getType() const noexcept;

		template<typename T> void setType() noexcept
		{
			type = typeid(T);
			factory = pointStateFactory<T>;
		}

		std::unique_ptr<State> createState() const;

		Port() : factory(nullptr), type(typeid(void)) {};
	};

	class DLLACTION NodeTemplate
	{
	public:
		class DLLACTION  ConfigData
		{
		private:
			std::type_index type;
			size_t size;
		public:
			std::type_index getType() const {
				return type;
			}

			size_t getSize() const noexcept {
				return size;
			}

			template<typename T> void setType() {
				type = typeid(T);
				size = sizeof(T);
			}

			ConfigData() : type(typeid(void)), size(0) {}
		};
		std::vector<Port> inputs,outputs;
		std::string action;
		ConfigData config;
		unsigned int flowInputPoints, flowOutputPoints;

		bool isFlowNode() const noexcept;
		bool isPureFlowNode() const noexcept;
		bool isComputationalNode() const noexcept;
		bool isPureComputationalNode() const noexcept;

		NodeTemplate() : flowInputPoints(0), flowOutputPoints(0) {}
	};

	class DLLACTION Node
	{
	public:
		struct DLLACTION PortWrapper
		{
		private:
			Node* node;
			unsigned port;
			bool flow;
			bool input;
		public:
			unsigned getPort() const noexcept;
			bool isFlowPort() const noexcept;
			bool isInput() const noexcept;
			const Node* getNode() const noexcept;
			Node* getNode() noexcept;
			bool isVoid() noexcept;

			bool operator == (const PortWrapper&) const noexcept;
			bool operator != (const PortWrapper&) const noexcept;

			PortWrapper operator >> (const PortWrapper& port);

			void connect(const PortWrapper&);

			PortWrapper(const Node* n,unsigned p,bool i, bool f);
			PortWrapper(const PortWrapper&) = default;

			static PortWrapper voidPort();
		};
		struct DLLACTION PortTypeWrapper
		{
			enum class Type
			{
				FlowOutput,
				FlowInput,
				ValueOutput,
				ValueInput
			};
		private:
			Node* node;
			Type type;
		public:

			PortWrapper get(unsigned port) const noexcept;
			inline PortWrapper operator | (unsigned port) const noexcept
			{
				return get(port);
			}

			PortTypeWrapper(Node* n, Type t);
			PortTypeWrapper(const PortTypeWrapper&) = default;
		};
	private:
		NodeTemplate* base;
		std::unique_ptr<State> config;
		std::unordered_map<unsigned, PortWrapper> inputPorts, flowOutputPorts;
		std::unordered_map<unsigned, std::vector<PortWrapper>> outputPorts, flowInputPorts;
		unsigned usedInputs;
	public:
		using PortType = PortTypeWrapper::Type;
		State* getConfig();

		bool connect(const PortWrapper& source,const PortWrapper& target) noexcept;
		bool disconnect(const PortWrapper& source,const PortWrapper& target = PortWrapper::voidPort()) noexcept;

		const NodeTemplate* getBase() const noexcept;

		PortWrapper getPort(unsigned id,bool input,bool flow) const;

		inline PortWrapper getFlowInputPort(unsigned id) const noexcept
		{
			return getPort(id, true, true);
		}
		inline PortWrapper getFlowOutputPort(unsigned id) const noexcept
		{
			return getPort(id,false,true);
		}
		inline PortWrapper getInputPort(unsigned id) const noexcept
		{
			return getPort(id,true,false);
		}
		inline PortWrapper getOutputPort(unsigned id) const noexcept
		{
			return getPort(id,false,false);
		}

		inline PortWrapper getInputPortTarget(unsigned id) const
		{
			auto it = inputPorts.find(id);
			if (it == inputPorts.end())
				return PortWrapper::voidPort();
			return it->second;
		}

		inline PortWrapper getFlowOutputPortTarget(unsigned id) const
		{
			auto it = flowOutputPorts.find(id);
			if (it == flowOutputPorts.end())
				return PortWrapper::voidPort();
			return it->second;
		}

		inline unsigned usedFlowInputs()
		{
			return usedInputs;
		}

		PortTypeWrapper operator [] (PortType portType) const noexcept;

		Node(NodeTemplate* nodeTemplate);
	};

	class DLLACTION Enviroment
	{
	public:
		std::vector<std::unique_ptr<Node>> nodes;
		std::vector<std::unique_ptr<NodeTemplate>> node_templates;

		void clear();

		Enviroment() = default;
		Enviroment(const Enviroment&) = delete;
		Enviroment(Enviroment&&) noexcept = default;
	};
}
