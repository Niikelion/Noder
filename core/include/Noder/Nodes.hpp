#pragma once

#include <vector>
#include <typeindex>
#include <exception>
#include <string>
#include <memory>
#include <unordered_map>
#include <type_traits>
#include <stdexcept>
#include <functional>
#include <algorithm>

#include "NodeUtils.hpp"
#include <parselib/XML/xml.hpp>
#include <parselib/JSON/json.hpp>

#define NoderDefaultSerializationFor(T) template<> struct Noder::ValueSerializer<T>: std::true_type\
{\
	std::string serialize(const T& value)\
	{\
		return Noder::_noder_hacks_::details::SerializeMethodWrapper<T>::toString(value);\
	}\
	T deserialize(const std::string& value)\
	{\
		return Noder::_noder_hacks_::details::DeserializeMethodWrapper<T>::deserialize(value);\
	}\
	T clone(const T& value)\
	{\
		return value;\
	}\
}

namespace Noder
{
	namespace _noder_hacks_
	{
		template<class...>struct types { using type = types; };
		template<class T>struct tag { using type = T; };
		template<class Tag>using type_t = typename Tag::type;

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

		namespace details
		{
			//serialize method
			template<typename T, typename = void> struct has_serialize_method : std::false_type {};
			template<typename T> struct has_serialize_method<T, std::enable_if<std::is_same<decltype(std::declval<const T&>().serialize()), std::string>::type::value>> : std::true_type {};

			template<typename T, typename = void> struct has_serialize_function : std::false_type {};
			template<typename T> struct has_serialize_function<T, std::enable_if_t<std::is_same<decltype(serialize(std::declval<const T&>())), std::string>::type::value>> : std::true_type {};

			template<typename T, typename = void> struct has_to_string_method : std::false_type {};
			template<typename T> struct has_to_string_method<T, std::enable_if_t<std::is_same<decltype(std::declval<const T&>().to_string()), std::string>::type::value>> : std::true_type {};

			template<typename T, typename = void> struct has_toString_method : std::false_type {};
			template<typename T> struct has_toString_method<T, std::enable_if_t<std::is_same<decltype(std::declval<const T&>().toString()), std::string>::type::value>> : std::true_type {};

			
			template<typename T, typename = void> struct has_to_string_function : std::false_type {};
			template<typename T> struct has_to_string_function<T, std::enable_if_t<std::is_same<decltype(std::to_string(std::declval<const T&>())), std::string>::type::value>> : std::true_type {};
			template<typename T> struct has_to_string_function<T, std::enable_if_t<std::is_same<decltype(to_string(std::declval<const T&>())), std::string>::type::value>> : std::true_type {};

			template<typename T, typename = void> struct SerializeMethodWrapper
			{
				static std::string toString(const T& value)
				{
					return "";
				}
			};

			template<typename T> struct SerializeMethodWrapper<T, std::enable_if_t<
				has_serialize_method<T>::value
				>>
			{
				static std::string toString(const T& value)
				{
					return value.serialize();
				}
			};

			template<typename T> using serialize_function_pre_cond = std::integral_constant<bool,!has_serialize_method<T>::value>;
			
			template<typename T> struct SerializeMethodWrapper<T, std::enable_if_t<
				serialize_function_pre_cond<T>::value && has_to_string_method<T>::value
				>>
			{
				static std::string toString(const T& value)
				{
					return serialize(value);
				}
			};

			template<typename T> using to_string_method_pre_cond = std::integral_constant<bool, serialize_function_pre_cond<T>::value && !has_serialize_function<T>::value>;

			template<typename T> struct SerializeMethodWrapper<T, std::enable_if_t<
				to_string_method_pre_cond<T>::value && has_to_string_method<T>::value
				>>
			{
				static std::string toString(const T& value)
				{
					return value.to_string();
				}
			};

			template<typename T> using toString_method_pre_cond = std::integral_constant<bool, to_string_method_pre_cond<T>::value && !has_to_string_method<T>::value>;

			template<typename T> struct SerializeMethodWrapper<T, std::enable_if_t<
				toString_method_pre_cond<T>::value && has_toString_method<T>::value
				>>
			{
				static std::string toString(const T& value)
				{
					return value.toString();
				}
			};

			template<typename T> using to_string_function_pre_cond = std::integral_constant<bool, toString_method_pre_cond<T>::value && !has_toString_method<T>::value>;

			template<typename T> struct SerializeMethodWrapper < T, std::enable_if_t<
				to_string_function_pre_cond<T>::value && has_to_string_function<T>::value
				>>
			{
				static std::string toString(const T& value)
				{
					using namespace std;
					return to_string(value);
				}
			};
			//deserialize method
			template<typename T, typename = void> struct has_deserialize_constructor : std::false_type {};
			template<typename T> struct has_deserialize_constructor<T, std::enable_if_t<std::is_same<decltype(T(std::string())), T>::type::value>> : std::true_type {};
		
			template<typename T, typename = void> struct has_deserialize_method : std::false_type {};
			template<typename T> struct has_deserialize_method<T, std::enable_if_t<std::is_same<decltype(std::declval<T&>().deserialize(std::declval<const std::string&>())), void>::type::value>> : std::true_type {};

			template<typename T, typename = void> struct has_deserialize_function : std::false_type {};
			template<typename T> struct has_deserialize_function<T, std::enable_if_t<std::is_same<decltype(deserialize(std::declval<T&>(),std::declval<const std::string&>())), void>::type::value>> : std::true_type {};
		
			template<typename T, typename = void> struct DeserializeMethodWrapper
			{
				static T deserialize(const std::string& source)
				{
					return T();
				}
			};

			template<typename T> struct DeserializeMethodWrapper<T, std::enable_if_t<
				has_deserialize_method<T>::value
				>>
			{
				static T deserialize(const std::string& source)
				{
					T value;
					value.deserialize(source);
					return std::move(value);
				}
			};

			template<typename T> struct DeserializeMethodWrapper<T, std::enable_if_t<
				!has_deserialize_method<T>::value&&
				has_deserialize_function<T>::value
				>>
			{
				static T deserialize(const std::string& source)
				{
					T value;
					deserialize(value, source);
					return std::move(value);
				}
			};

			template<typename T> struct DeserializeMethodWrapper<T,std::enable_if_t<
				!has_deserialize_method<T>::value &&
				!has_deserialize_function<T>::value &&
				has_deserialize_constructor<T>::value
				>>
			{
				static T deserialize(const std::string& source)
				{
					return T(source);
				}
			};
		}
	}

	using namespace ParseLib;

	class DLLACTION TypeMismatchException : public std::exception
	{
	private:
		std::string msg;
	public:
		virtual const char* what() const noexcept;
		TypeMismatchException(const std::type_index& source, const std::type_index& target) noexcept;
	};

	template<typename T, typename = void> struct ValueSerializer : std::false_type {};

	template<typename T> bool supportsSerialization(const T& t)
	{
		return ValueSerializer<T>::value;
	}

	template<typename T> bool supportsSerialization()
	{
		return ValueSerializer<T>::value;
	}

	template<typename T> struct ValueSerializer<T, std::enable_if_t<std::is_integral<T>::value>> : std::true_type
	{
		std::string serialize(const T& value)
		{
			return std::to_string(value);
		}
		T deserialize(const std::string& value)
		{
			return static_cast<T>(std::stoll(value));
		}
		T clone(const T& value)
		{
			return value;
		}
	};

	template<typename T> struct ValueSerializer<T, std::enable_if_t<std::is_floating_point<T>::value>> : std::true_type
	{
		std::string serialize(const T& value)
		{
			return std::to_string(value);
		}
		T deserialize(const std::string& value)
		{
			return static_cast<T>(std::stold(value));
		}
		T clone(const T& value)
		{
			return value;
		}
	};

	template<> struct ValueSerializer<std::string> : std::true_type
	{
		std::string serialize(const std::string& value)
		{
			return value;
		}
		std::string deserialize(const std::string& value)
		{
			return value;
		}
		std::string clone(const std::string& value)
		{
			return value;
		}
	};

	template<> struct ValueSerializer<JSON::Value::Type> : std::true_type
	{
		std::string serialize(const JSON::Value::Type& value)
		{
			return JSON::stringify(value);
		}
		JSON::Value::Type deserialize(const std::string& value)
		{
			return JSON::parse(value);
		}
	};

	class DLLACTION State
	{
	private:
		std::vector<char> data;
		void* pointer;
		std::type_index info;
		void(*deleter)(void*);
		std::string(*serializer)(void*);
		void(*deserializer)(void*&, char*, const std::string&);

		template<typename T, typename Enabled = void> struct funcProvider
		{
			static void deleterFunc(void* p)
			{
				reinterpret_cast<T*>(p)->~T();
			}
			static std::string serializerFunc(void* p)
			{
				throw std::logic_error(std::string("Serialization is disabled for type ") + typeid(T).name());
				return "";
			}
			static void deserializerFunc(void*& p, char* buffer, const std::string& value)
			{
				throw std::logic_error(std::string("Serialization is disabled for type ") + typeid(T).name());
			}
		};

		template<typename T> struct funcProvider<T, std::enable_if_t<ValueSerializer<T>::value>>
		{
			static void deleterFunc(void* p)
			{
				reinterpret_cast<T*>(p)->~T();
			}
			static std::string serializerFunc(void* p)
			{
				return ValueSerializer<T>().serialize(*reinterpret_cast<T*>(p));
			}
			static void deserializerFunc(void*& p, char* buffer, const std::string& value)
			{
				if (p == nullptr)
				{
					p = new(buffer) T(ValueSerializer<T>().deserialize(value));
				}
				else
					(*reinterpret_cast<T*>(p)) = ValueSerializer<T>().deserialize(value);
			}
		};
	public:
		template<typename T> void setValue(const T& value)
		{
			if (checkType<T>())
			{
				if (pointer == nullptr)
				{
					pointer = new(data.data()) T(value);
					deleter = funcProvider<T>::deleterFunc;
					serializer = funcProvider<T>::serializerFunc;
					deserializer = funcProvider<T>::deserializerFunc;
				}
				else
					*reinterpret_cast<T*>(pointer) = value;
			}
		}

		template<typename T, typename ...Args> void emplaceValue(Args... args)
		{
			if (pointer == nullptr && checkType<T>())
			{
				pointer = new(data.data()) T(std::forward<Args>(args)...);
				deleter = funcProvider<T>::deleterFunc;
				serializer = funcProvider<T>::serializerFunc;
				deserializer = funcProvider<T>::deserializerFunc;
			}
		}

		template<typename T> bool checkType() const noexcept
		{
			return info == typeid(T);
		}

		bool isReady() const noexcept;
		void reset();

		std::string serialize() const;
		void deserialize(const std::string& value);

		template<typename T> T& getValue()
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

		template<typename T> State(_noder_hacks_::tag<T>) : pointer(nullptr),
			deleter(funcProvider<T>::deleterFunc),
			serializer(funcProvider<T>::serializerFunc),
			deserializer(funcProvider<T>::deserializerFunc),
			info(typeid(T))
		{
			data.resize(sizeof(T));
		}
		State(const std::type_index& type, size_t size) : pointer(nullptr), deleter(nullptr), serializer(nullptr), deserializer(nullptr), info(type)
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
	public:
		std::type_index getType() const noexcept;

		template<typename T> void setType() noexcept
		{
			type = typeid(T);
			factory = pointStateFactory<T>;
		}

		std::unique_ptr<State> createState() const;

		Port() : factory(nullptr), type(typeid(void)) {};
		template<typename T> Port(_noder_hacks_::tag<T>) : type(typeid(T)), factory(pointStateFactory<T>) {}
	private:
		template<typename T> static std::unique_ptr<State> pointStateFactory()
		{
			return std::make_unique<State>(State(_noder_hacks_::tag<T>{}));
		}
		std::type_index type;
		std::unique_ptr<State>(*factory)();
	};

	//TODO: encapsulate members, make them assignable only by constructor to prevent runtime confusion
	class DLLACTION NodeTemplate
	{
	public:
		using Ptr = std::shared_ptr<NodeTemplate>;

		std::vector<Port> inputs, outputs;
		std::string action;
		Port config;
		unsigned int flowInputPoints, flowOutputPoints;

		bool isFlowNode() const noexcept;
		bool isPureFlowNode() const noexcept;
		bool isComputationalNode() const noexcept;
		bool isPureComputationalNode() const noexcept;

		NodeTemplate() : flowInputPoints(0), flowOutputPoints(0) {}
	};

	//TODO: remove inlines
	class DLLACTION Node : private std::enable_shared_from_this<Node>
	{
	public:
		using Ptr = std::shared_ptr<Node>;

		struct DLLACTION PortWrapper
		{
		public:
			unsigned getPort() const noexcept;
			bool isFlowPort() const noexcept;
			bool isInput() const noexcept;
			Node& getNode() const;
			bool isVoid() const noexcept;

			bool operator == (const PortWrapper&) const noexcept;
			bool operator != (const PortWrapper&) const noexcept;

			PortWrapper& operator >> (PortWrapper& port);
			PortWrapper& operator << (PortWrapper& port);

			void connect(const PortWrapper&);

			PortWrapper(Node* n, unsigned p, bool i, bool f);
			PortWrapper(const PortWrapper&) = default;

			static PortWrapper voidPort();
		private:
			Node* node;
			unsigned port;
			bool flow;
			bool input;
		};
		struct DLLACTION ConstPortWrapper
		{
		public:
			unsigned getPort() const noexcept;
			bool isFlowPort() const noexcept;
			bool isInput() const noexcept;
			const Node& getNode() const noexcept;
			bool isVoid() const noexcept;

			bool operator == (const ConstPortWrapper&) const noexcept;
			bool operator != (const ConstPortWrapper&) const noexcept;

			ConstPortWrapper(const PortWrapper& p);
			ConstPortWrapper(const Node* n, unsigned p, bool i, bool f);
			ConstPortWrapper(const ConstPortWrapper&) = default;

			static ConstPortWrapper voidPort();
		private:
			const Node* node;
			unsigned port;
			bool flow;
			bool input;
		};

		struct DLLACTION PortTypeWrapper
		{
			//TODO: move to another place, possibly split into 2 enums?
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
			inline PortWrapper operator [] (unsigned port) const noexcept
			{
				return get(port);
			}

			PortTypeWrapper(Node* n, Type t);
			PortTypeWrapper(const PortTypeWrapper&) = default;
		};
		struct DLLACTION ConstPortTypeWrapper
		{
		private:
			const Node* node;
			PortTypeWrapper::Type type;
		public:

			ConstPortWrapper get(unsigned port) const noexcept;
			inline ConstPortWrapper operator [] (unsigned port) const noexcept
			{
				return get(port);
			}

			ConstPortTypeWrapper(const Node* n, PortTypeWrapper::Type t);
			ConstPortTypeWrapper(const ConstPortTypeWrapper&) = default;
		};
	private:
		NodeTemplate::Ptr base;
		std::unique_ptr<State> config;
		std::vector<std::unique_ptr<State>> inputValues;
		std::unordered_map<unsigned, PortWrapper> inputPorts, flowOutputPorts;
		std::unordered_map<unsigned, std::vector<PortWrapper>> outputPorts, flowInputPorts;
		unsigned usedInputs;
	public:
		using PortType = PortTypeWrapper::Type;
		State* getConfig() const;

		template<typename T> void setInputValue(unsigned id, const T& value)
		{
			if (id < inputValues.size())
			{
				inputValues[id] = base->inputs[id].createState();
				inputValues[id]->emplaceValue<T>(value);
			}
		}

		void deserializeInputValue(unsigned id, const std::string& value);

		const PortState* getInputState(unsigned id) const;

		bool connect(const PortWrapper& source, const PortWrapper& target) noexcept;
		bool disconnect(const PortWrapper& source, const PortWrapper& target = PortWrapper::voidPort()) noexcept;

		const std::shared_ptr<NodeTemplate> getBase() const noexcept;

		PortWrapper getPort(unsigned id, bool input, bool flow);
		ConstPortWrapper getPort(unsigned id, bool input, bool flow) const;

		PortWrapper getFlowInputPort(unsigned id)
		{
			return getPort(id, true, true);
		}
		ConstPortWrapper getFlowInputPort(unsigned id) const
		{
			return getPort(id, true, true);
		}

		PortWrapper getFlowOutputPort(unsigned id)
		{
			return getPort(id, false, true);
		}
		ConstPortWrapper getFlowOutputPort(unsigned id) const
		{
			return getPort(id, false, true);
		}

		PortWrapper getInputPort(unsigned id)
		{
			return getPort(id, true, false);
		}
		ConstPortWrapper getInputPort(unsigned id) const
		{
			return getPort(id, true, false);
		}

		PortWrapper getOutputPort(unsigned id)
		{
			return getPort(id, false, false);
		}
		ConstPortWrapper getOutputPort(unsigned id) const
		{
			return getPort(id, false, false);
		}

		inline PortWrapper getInputPortTarget(unsigned id)
		{
			auto it = inputPorts.find(id);
			if (it == inputPorts.end())
				return PortWrapper::voidPort();
			return it->second;
		}

		inline ConstPortWrapper getInputPortTarget(unsigned id) const
		{
			auto it = inputPorts.find(id);
			if (it == inputPorts.end())
				return ConstPortWrapper::voidPort();
			return it->second;
		}

		inline PortWrapper getFlowOutputPortTarget(unsigned id)
		{
			auto it = flowOutputPorts.find(id);
			if (it == flowOutputPorts.end())
				return PortWrapper::voidPort();
			return it->second;
		}

		inline ConstPortWrapper getFlowOutputPortTarget(unsigned id) const
		{
			auto it = flowOutputPorts.find(id);
			if (it == flowOutputPorts.end())
				return ConstPortWrapper::voidPort();
			return it->second;
		}

		inline std::vector<PortWrapper> getFlowInputTargets(unsigned id)
		{
			auto it = flowInputPorts.find(id);
			if (it == flowInputPorts.end())
				return {};
			return it->second;
		}

		inline std::vector<ConstPortWrapper> getFlowInputTarget(unsigned id) const
		{
			auto it = flowInputPorts.find(id);
			if (it == flowInputPorts.end())
				return {};
			std::vector<ConstPortWrapper> ret;
			ret.reserve(it->second.size());
			for (size_t i = 0; i < ret.size(); ++i)
				ret.emplace_back(it->second[i]);
			return ret;
		}

		unsigned usedFlowInputs() const
		{
			return usedInputs;
		}

		PortTypeWrapper operator [] (PortType portType) noexcept;
		ConstPortTypeWrapper operator [] (PortType portType) const noexcept;

		Node(const std::shared_ptr<NodeTemplate>& nodeTemplate);
	};

	class DLLACTION Enviroment
	{
	public:
		std::vector<std::shared_ptr<NodeTemplate>> nodeTemplates;
		std::vector<std::shared_ptr<Node>> nodes;


		void clearNodes();
		void clear();

		std::unique_ptr<XML::Tag> exportToXML() const;
		void importFromXML(const std::unique_ptr<XML::Tag>& tag);

		Node::Ptr createNode(const NodeTemplate::Ptr& base);
		NodeTemplate::Ptr createTemplate();
		NodeTemplate::Ptr createTemplate(const std::string& name, const std::vector<Port>& inP, const std::vector<Port>& outP, unsigned flowInP, unsigned flowOutP);

		Enviroment() = default;
		Enviroment(const Enviroment&) = delete;
		Enviroment(Enviroment&&) noexcept = default;
	};

	namespace TypeUtils
	{
		namespace details
		{
			template<unsigned> void _unpackPorts(std::vector<Port>&) {}
			template<unsigned, typename T, typename... Args> void _unpackPorts(std::vector<Port>& p)
			{
				p.emplace_back(_noder_hacks_::tag<T>{});
				_unpackPorts<0, Args...>(p);
			}
		}

		template<typename... Args> void unpackPorts(std::vector<Port>& p)
		{
			details::_unpackPorts<0, Args...>(p);
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
			template<typename T, size_t... I> static std::tuple<Rets...> call(
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
	}
}
