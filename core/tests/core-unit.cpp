#include <catch.hpp>
#include <cmath>
#include <string>
#include <tuple>
#include <sstream>

#include <iostream>

#include <Noder/Nodes.hpp>

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