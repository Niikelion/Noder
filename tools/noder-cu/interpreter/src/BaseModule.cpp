#include <Noder/Interpreter.hpp>

#ifdef _WIN32
#define DLL_EXPORT extern "C" __declspec(dllexport)
#elif
#define DLL_EXPORT extern "C"
#endif

DLL_EXPORT void exportActions(Noder::NodeInterpreter& inter)
{
	typedef float(*funcf2f)(float);
	typedef float(*funcff2f)(float, float);

	inter.createTemplate([]() {return std::tuple<>(); }, "start").flowOutputPoints = 1;
	Noder::NodeTemplate& printT = inter.createTemplate([](std::string value) { printf("%s\n", value.c_str()); }, "print");
	printT.flowInputPoints = 1;
	printT.flowOutputPoints = 1;

	inter.createTemplate((funcff2f)pow, "pow");
}