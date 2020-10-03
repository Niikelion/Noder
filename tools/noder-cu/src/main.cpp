#include <iostream>
#include <cstdlib>
#include <cmath>
#include <unordered_set>

#include <cxxopts.hpp>
#include <termcolor.hpp>
#include <nhash.hpp>

#include <Interpreter.hpp>
#include <creator.hpp>

template<typename T> using P = Noder::Pointer<T>;
using Port = Noder::Node::PortType;

constexpr uint32_t operator"" _hashed(char const* s, size_t count)
{
	return nhash::constant::fnv1a_32(s, count);
}

uint32_t hash32(const std::string& s)
{
	return nhash::fnv1a_32(s);
}

int main(int argc, const char** argv)
{
	std::unordered_set<std::string> modes = {
		"interpreter"
	};

	cxxopts::Options options("noder-cu", "Console utilities from Noder Toolkit");

	options.add_options()
		("h,help", "Print usage")
		("v,verbose", "Verbose output", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
		("no-definitions", "Start without loading default definitions", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
		("no-color", "Disable output coloring", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
		("modules", "Load specified modules", cxxopts::value<std::vector<std::string>>())
		("mode", "Available modes: 'interpreter'", cxxopts::value<std::string>())
		("c,creator", "Enter enviroment interactive creator before passing it to module", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
		("file", "Load project from file", cxxopts::value<std::string>())
		("e,execute", "Execute enviroment", cxxopts::value<bool>()->default_value("true"))
		("output", "Save enviroment to file", cxxopts::value<std::string>())
		;

	try
	{
		auto result = options.parse(argc, argv);

		if (result.count("help"))
		{
			std::cout << options.help() << std::endl;
		}
		bool useColor = !result["no-color"].as<bool>();
		bool verbose = result["verbose"].as<bool>();

		if (result.count("mode") == 0)
		{
			if (verbose)
			{
				if (useColor)
					std::cout << termcolor::yellow;

				std::cout << "No mode was specified." << std::endl;

				if (useColor)
					std::cout << termcolor::reset;

			}
			return 1;
		}

		std::string mode = result["mode"].as<std::string>();

		if (modes.find(mode) == modes.end())
		{
			if (verbose)
			{
				if (useColor)
					std::cout << termcolor::yellow;

				std::cout << "Mode: \"" << mode << "\" is not supported.\n";

				if (useColor)
					std::cout << termcolor::reset;
			}
			return 1;
		}

		switch (hash32(mode))
		{
		case "interpreter"_hashed:
		{
			Noder::Tools::Interpreter interpreter;
			if (!result["no-definitions"].as<bool>())
			{
				interpreter.loadDefaultDefinitions();
			}

			if (result.count("file"))
			{
				std::string file = result["file"].as<std::string>();
				if (file.rfind(".nbf") == file.length() - std::string(".nbf").length())
				{
					//binary file
				}
				else
				{
					interpreter.loadEnviromentFromXmlFile(file);
				}
			}
			
			if (result["creator"].as<bool>())
			{
				if (verbose)
					std::cout << "Running creator's python console.\nEnviroment is exposed via 'env' variable.\n";
				Creator creator(interpreter.getEnviroment());
				creator.run();
			}

			if (result.count("output"))
			{
				if (verbose)
					std::cout << "Saving enviroment.\n";
				interpreter.saveEnviromentToXmlFile(result["output"].as<std::string>());
			}

			if (result["execute"].as<bool>())
			{
				if (verbose)
					std::cout << "Running interpreter.\n";
				interpreter.prepare();
				interpreter.run();
			}
			break;
		}
		}
	}
	catch (cxxopts::OptionException& e)
	{
		std::cerr << e.what() << std::endl;
	}
	return 0;
}