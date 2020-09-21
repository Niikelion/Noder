#include <catch.hpp>
#include <cmath>
#include <string>
#include <tuple>
#include <sstream>

#include <iostream>

#include <Noder/Interpreter.hpp>

using float_overload = float(*)(float);

using StateAccess = Noder::NodeInterpreter::NodeState::Access;

void setExternalFloat(StateAccess access, float value)
{
	access.getConfigValue<float>() = value;
}

std::tuple<float, float> swapFloat(float a, float b)
{
	return { b, a };
}

std::tuple<std::string, float> readStringFromConfig(StateAccess access)
{
	std::string value = access.getConfigValue<std::string>();
	return { value, (float)value.size() };
}

void printString(StateAccess access,std::string s)
{
	access.getConfigValue<std::stringstream>() << s;
}

struct NotSerializableType
{
	NotSerializableType(int a) {}
};

struct DefaultSerializableType
{
public:
	int value;

	std::string toString() const
	{
		return std::to_string(value);
	}
	void deserialize(const std::string& source)
	{
		value = std::stoi(source);
	}
	DefaultSerializableType() : value(0) {}
	DefaultSerializableType(int x) : value(x) {}
	DefaultSerializableType(const DefaultSerializableType& d) = default;
};

NoderDefaultSerializationFor(DefaultSerializableType);

class LogicErrorMatcher : public Catch::MatcherBase<std::logic_error>
{
private:
	std::string message;
public:
	LogicErrorMatcher(const std::string& m) :
		message(m)
	{}

	bool match(std::logic_error const& le) const override
	{
		return Catch::StartsWith(message).match(le.what());
	}

	std::string describe() const override { return ""; }
};

TEST_CASE("Core - serialization utils", "[unit],[core]")
{
	//TODO: check deserialization
	using namespace Noder;

	INFO("Check default types that are required to have serialization enabled");

	REQUIRE(supportsSerialization<int>());
	REQUIRE(supportsSerialization<unsigned int>());
	REQUIRE(supportsSerialization<float>());
	REQUIRE(supportsSerialization<std::string>());
	REQUIRE(supportsSerialization<JSON::Value::Type>());

	State intState(_noder_hacks_::tag<int>{});
	intState.deserialize("1");
	REQUIRE(intState.isReady());
	REQUIRE(intState.getValue<int>() == 1);

	INFO("Check user created types");

	REQUIRE(!supportsSerialization<NotSerializableType>());
	REQUIRE(supportsSerialization<DefaultSerializableType>());

	State nVState(_noder_hacks_::tag<NotSerializableType>{});
	nVState.emplaceValue<NotSerializableType>(5);
	REQUIRE_THROWS_MATCHES(nVState.serialize(),std::logic_error,LogicErrorMatcher("Serialization is disabled"));

	DefaultSerializableType v(5);
	State vState(_noder_hacks_::tag<DefaultSerializableType>{});
	vState.setValue(v);
	REQUIRE(vState.serialize() == "5");
}

TEST_CASE("Interpreter - node creation and execution", "[unit],[interpreter]")
{
	const std::string str = "test";

	using namespace Noder;
	INFO("Create interpreter instance.");

	NodeInterpreter interpreter;

	INFO("Create node definitions using existing functions.");

	NodeTemplate& sinT = interpreter.createTemplate((float_overload)sin);
	NodeTemplate& swapT = interpreter.createTemplate(swapFloat);
	NodeTemplate& floatSetterT = interpreter.createTemplateWithStates(setExternalFloat);
	NodeTemplate& stringReaderT = interpreter.createTemplateWithStates(readStringFromConfig);
	NodeTemplate& printStringT = interpreter.createTemplateWithStates(printString);

	floatSetterT.config.setType<float>();
	floatSetterT.flowInputPoints = 1;
	floatSetterT.flowOutputPoints = 1;

	stringReaderT.config.setType<std::string>();

	printStringT.config.setType<std::stringstream>();
	printStringT.flowInputPoints = 1;
	printStringT.flowOutputPoints = 1;

	INFO("Create node definitions using functors or lambdas.");

	float value = 0;

	NodeTemplate& constantReaderT = interpreter.createTemplate([value]() 
		{
			return value;
		});

	NodeTemplate& l_sinT = interpreter.createTemplate([](float x) 
		{
			return sin(x);
		});
	NodeTemplate& l_swapT = interpreter.createTemplate([](float a, float b)
		{
			return std::make_tuple(b, a);
		});
	NodeTemplate& l_floatSetterT = interpreter.createTemplateWithStates([](StateAccess access, float x)
		{
			access.getConfigValue<float>() = x;
		});
	NodeTemplate& l_stringReaderT = interpreter.createTemplateWithStates([](StateAccess access)
		{
			std::string v = access.getConfigValue<std::string>();
			return std::make_tuple(v, (float)v.size());
		});
	NodeTemplate& l_printStringT = interpreter.createTemplateWithStates([](StateAccess access, std::string s) 
		{
			access.getConfigValue<std::stringstream>() << s;
		});


	l_floatSetterT.config.setType<float>();
	l_floatSetterT.flowInputPoints = 1;
	l_floatSetterT.flowOutputPoints = 1;

	l_stringReaderT.config.setType<std::string>();

	l_printStringT.config.setType<std::stringstream>();
	l_printStringT.flowInputPoints = 1;
	l_printStringT.flowOutputPoints = 1;

	INFO("Create nodes and initialize configs.");
	//nodes from templates using functions

	Node& constantReaderN1 = interpreter.createNode(constantReaderT);

	Node& sinN1 = interpreter.createNode(sinT);
	Node& swapN1 = interpreter.createNode(swapT);
	Node& floatSetterN1 = interpreter.createNode(floatSetterT);
	Node& floatSetterN2 = interpreter.createNode(floatSetterT);
	Node& stringReaderN1 = interpreter.createNode(stringReaderT);
	Node& printStringN1 = interpreter.createNode(printStringT);

	floatSetterN1.getConfig()->setValue<float>(0);
	floatSetterN2.getConfig()->setValue<float>(0);
	stringReaderN1.getConfig()->setValue<std::string>(str);
	printStringN1.getConfig()->emplaceValue<std::stringstream>();

	//nodes from templates using functors or lambdas

	Node& l_sinN1 = interpreter.createNode(l_sinT);
	Node& l_swapN1 = interpreter.createNode(l_swapT);
	Node& l_floatSetterN1 = interpreter.createNode(l_floatSetterT);
	Node& l_floatSetterN2 = interpreter.createNode(l_floatSetterT);
	Node& l_stringReaderN1 = interpreter.createNode(l_stringReaderT);
	Node& l_printStringN1 = interpreter.createNode(l_printStringT);

	l_floatSetterN1.getConfig()->setValue<float>(0);
	l_floatSetterN2.getConfig()->setValue<float>(0);
	l_stringReaderN1.getConfig()->setValue<std::string>(str);
	l_printStringN1.getConfig()->emplaceValue<std::stringstream>();

	INFO("Setup connections.");

	//mostly non functor network
	
	REQUIRE_NOTHROW([&]()
		{
			stringReaderN1.getOutputPort(0) >> printStringN1.getInputPort(0);
			stringReaderN1.getOutputPort(1) >> swapN1.getInputPort(0);
			constantReaderN1.getOutputPort(0) >> swapN1.getInputPort(1);
			swapN1.getOutputPort(0) >> sinN1.getInputPort(0);
			sinN1.getOutputPort(0) >> floatSetterN1.getInputPort(0);
			swapN1.getOutputPort(1) >> floatSetterN2.getInputPort(0);

			printStringN1.getFlowOutputPort(0) >> floatSetterN1.getFlowInputPort(0);
			floatSetterN1.getFlowOutputPort(0) >> floatSetterN2.getFlowInputPort(0);
		}());

	//pure functor network

	REQUIRE_NOTHROW([&]()
		{
			l_stringReaderN1.getOutputPort(0) >> l_printStringN1.getInputPort(0);
			l_stringReaderN1.getOutputPort(1) >> l_swapN1.getInputPort(0);
			constantReaderN1.getOutputPort(0) >> l_swapN1.getInputPort(1);
			l_swapN1.getOutputPort(0) >> l_sinN1.getInputPort(0);
			l_sinN1.getOutputPort(0) >> l_floatSetterN1.getInputPort(0);
			l_swapN1.getOutputPort(1) >> l_floatSetterN2.getInputPort(0);

			l_printStringN1.getFlowOutputPort(0) >> l_floatSetterN1.getFlowInputPort(0);
			l_floatSetterN1.getFlowOutputPort(0) >> l_floatSetterN2.getFlowInputPort(0);
		}());

	INFO("Run both node programs and compare results");

	REQUIRE_NOTHROW([&]()
		{
			interpreter.runFrom(printStringN1);
			interpreter.runFrom(l_printStringN1);
		}());

	REQUIRE( printStringN1.getConfig()->getValue<std::stringstream>().str() == str );
	REQUIRE( l_printStringN1.getConfig()->getValue<std::stringstream>().str() == str );
}

TEST_CASE("Interpreter - flow redirection", "[unit],[interpreter]")
{
	//TODO: finish
	using namespace Noder;
	INFO("Create and setup interpreter instance and templates.");

	NodeInterpreter interpreter;

	NodeTemplate& readExternalIntT = interpreter.createTemplate();
}