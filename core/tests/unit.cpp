#include <catch.hpp>
#include <cmath>
#include <string>
#include <tuple>
#include <sstream>

#include <iostream>

#include <Noder/Interpreter.hpp>

using float_overload = float(*)(float);

class ExternalFloatSetter : public Noder::NodeInterpreter::ObjectNodeState<void(float)>
{
private:
	float* x;
public:
	virtual void calculate(float value) override
	{
		if (x != nullptr)
			*x = value;
	}

	void setVariable(float& v)
	{
		x = &v;
	}

	ExternalFloatSetter(const Noder::Node& node, std::unique_ptr<Noder::State>& scoped) : ObjectNodeState(node, scoped), x(nullptr) {}
};

std::tuple<float, float> swapFloat(float a, float b)
{
	return { b, a };
}

class ConfigStringReader : public Noder::NodeInterpreter::ObjectNodeState<std::tuple<std::string, float>()>
{
private:
	const Noder::Node& node;
public:
	virtual std::tuple<std::string, float> calculate() override
	{
		std::string value = const_cast<Noder::Node&>(node).getConfig()->getValue<std::string>();
		return { value, (float)value.size() };
	}
	ConfigStringReader(const Noder::Node& n, std::unique_ptr<Noder::State>& scoped) : ObjectNodeState(n, scoped), node(n) {}
};

class StringPrinter : public Noder::NodeInterpreter::ObjectNodeState<void(std::string)>
{
private:
	const Noder::Node& node;
public:
	virtual void calculate(std::string s) override
	{
		const_cast<Noder::Node&>(node).getConfig()->getValue<std::stringstream>() << s;
	}

	StringPrinter(const Noder::Node& n, std::unique_ptr<Noder::State>& scoped) : ObjectNodeState(n, scoped), node(n) {}
};

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

TEST_CASE("Interpreter - non functor node creation and execution", "[unit],[interpreter]")
{
	const std::string str = "test";

	using namespace Noder;
	INFO("Create interpreter instance.");

	NodeInterpreter interpreter;

	INFO("Create node definitions using existing functions and classes.");

	NodeTemplate& sinT = interpreter.createTemplate((float_overload)sin);
	NodeTemplate& swapT = interpreter.createTemplate(swapFloat);
	NodeTemplate& floatSetterT = interpreter.createTemplate<ExternalFloatSetter>();
	NodeTemplate& stringReaderT = interpreter.createTemplate<ConfigStringReader>();
	NodeTemplate& printStringT = interpreter.createTemplate<StringPrinter>();

	floatSetterT.config.setType<float>();
	floatSetterT.flowInputPoints = 1;
	floatSetterT.flowOutputPoints = 1;

	stringReaderT.config.setType<std::string>();

	printStringT.config.setType<std::stringstream>();
	printStringT.flowInputPoints = 1;
	printStringT.flowOutputPoints = 1;

	INFO("Create nodes and initialize configs.");
	//nodes from templates using functions

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

	INFO("Setup connections.");

	//mostly non functor network

	REQUIRE_NOTHROW([&]()
		{
			stringReaderN1.getOutputPort(0) >> printStringN1.getInputPort(0);
			stringReaderN1.getOutputPort(1) >> swapN1.getInputPort(0);
			swapN1.setInputValue<float>(1,0.f);
			swapN1.getOutputPort(0) >> sinN1.getInputPort(0);
			sinN1.getOutputPort(0) >> floatSetterN1.getInputPort(0);
			swapN1.getOutputPort(1) >> floatSetterN2.getInputPort(0);

			printStringN1.getFlowOutputPort(0) >> floatSetterN1.getFlowInputPort(0);
			floatSetterN1.getFlowOutputPort(0) >> floatSetterN2.getFlowInputPort(0);
		}());

	//pure functor network

	INFO("Run both node programs and compare results");

	REQUIRE_NOTHROW([&]()
		{
			interpreter.runFrom(printStringN1);
		}());

	REQUIRE( printStringN1.getConfig()->getValue<std::stringstream>().str() == str );
}

TEST_CASE("Interpreter - flow redirection", "[unit],[interpreter]")
{
	//TODO: finish
	using namespace Noder;
	INFO("Create and setup interpreter instance and templates.");

	NodeInterpreter interpreter;

	NodeTemplate& readExternalIntT = interpreter.createTemplate();
}