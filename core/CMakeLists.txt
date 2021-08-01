#config variables

set(ALL_MODULES "interpreter;compiler")

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

option(NODER_MODULES "selected modules or 'all'" "all")

if (NODER_MODULES STREQUAL "all")
	set(NODER_MODULES ${ALL_MODULES})
endif()

#sources

set(CORE_SOURCES
	src/Nodes.cpp
)

set(CORE_HEADERS
	include/Noder/Nodes.hpp
	include/Noder/NodeUtils.hpp
)

set(INTERPRETER_SOURCES
	src/Interpreter.cpp
)

set(INTERPRETER_HEADERS
	include/Noder/Interpreter.hpp
)

set(COMPILER_SOURCES
	src/Compiler.cpp
)

set(COMPILER_HEADERS
	include/Noder/Compiler.hpp
)

find_package(parselib REQUIRED)

if ("compiler" IN_LIST NODER_MODULES)
	option(LLVM_CONFIG "Path to llvm-config" "")
	option(LLVM_CONFIG_D "" CACHE FILEPATH "Path to llvm-config(debug)")
	
#	set(LLVM_MODULES "codegen interpreter executionengine linker mcjit mcparser native tablegen")
	set(LLVM_MODULES "all")

	if (LLVM_CONFIG STREQUAL "")
		message(FATAL_ERROR "You must specify llvm-config path")
	endif()

	execute_process(COMMAND ${LLVM_CONFIG}
		--includedir OUTPUT_VARIABLE llvm_include_dir
		OUTPUT_STRIP_TRAILING_WHITESPACE)

	execute_process(COMMAND ${LLVM_CONFIG} 
		--prefix OUTPUT_VARIABLE llvm_root
		OUTPUT_STRIP_TRAILING_WHITESPACE)

	execute_process(COMMAND ${LLVM_CONFIG} 
		--system-libs OUTPUT_VARIABLE sys_libs
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	separate_arguments(sys_libs)

	execute_process(COMMAND ${LLVM_CONFIG}
		--link-static --libfiles ${LLVM_MODULES}
		OUTPUT_VARIABLE llvm_libs OUTPUT_STRIP_TRAILING_WHITESPACE)
	separate_arguments(llvm_libs)

	execute_process(COMMAND ${LLVM_CONFIG}
		--cppflags
		OUTPUT_VARIABLE llvm_flags OUTPUT_STRIP_TRAILING_WHITESPACE)
	separate_arguments(llvm_flags)

	if (isMultiConfig)
		if (LLVM_CONFIG_D STREQUAL "")
			message(FATAL_ERROR "You must specify llvm-config path for debug configuration")
		endif()
		execute_process(COMMAND ${LLVM_CONFIG_D}
			--link-static --libfiles ${LLVM_MODULES}
			OUTPUT_VARIABLE llvm_libs_d OUTPUT_STRIP_TRAILING_WHITESPACE)
		separate_arguments(llvm_libs_d)
		execute_process(COMMAND ${LLVM_CONFIG_D}
			--cppflags
			OUTPUT_VARIABLE llvm_flags_d OUTPUT_STRIP_TRAILING_WHITESPACE)

		separate_arguments(llvm_flags_d)
	endif()
endif()

#TODO: make target specific header installing
install(DIRECTORY include/Noder DESTINATION include)

#core library

add_library(noder-core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
set_target_properties(noder-core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(noder-core PUBLIC parselib::xml parselib::json)
target_include_directories(noder-core PUBLIC include)

set_target_properties(noder-core PROPERTIES FOLDER "Libs")
if (isMultiConfig)
	install(TARGETS noder-core DESTINATION "$<CONFIG>/lib")
else()
	install(TARGETS noder-core DESTINATION ${CMAKE_BUILD_TYPE}/lib)
endif()

#interpreter module
if ("interpreter" IN_LIST NODER_MODULES)
	add_library(noder-interpreter STATIC ${INTERPRETER_SOURCES} ${INTERPRETER_HEADERS})
	set_target_properties(noder-interpreter PROPERTIES POSITION_INDEPENDENT_CODE ON)
	target_link_libraries(noder-interpreter PUBLIC noder-core)
	target_include_directories(noder-interpreter PUBLIC include)
	
	set_target_properties(noder-interpreter PROPERTIES FOLDER "Libs")
	if (isMultiConfig)
	install(TARGETS noder-interpreter DESTINATION "$<CONFIG>/lib")
	else()
		install(TARGETS noder-interpreter DESTINATION ${CMAKE_BUILD_TYPE}/lib)
	endif()
endif()
#compiler module
if ("compiler" IN_LIST NODER_MODULES)
	add_library(noder-compiler STATIC ${COMPILER_SOURCES} ${COMPILER_HEADERS})
	set_target_properties(noder-compiler PROPERTIES POSITION_INDEPENDENT_CODE ON)
	target_link_libraries(noder-compiler PUBLIC noder-core ${sys_libs})
	target_include_directories(noder-compiler PUBLIC include ${llvm_include_dir} ${llvm_root}/include)

	if (isMultiConfig)
		target_link_libraries(noder-compiler PUBLIC "$<$<CONFIG:Debug>:${llvm_libs_d}>$<$<NOT:$<CONFIG:Debug>>:${llvm_libs}>")
		target_compile_options(noder-compiler PUBLIC "$<$<CONFIG:Debug>:${llvm_flags_d}>$<$<NOT:$<CONFIG:Debug>>:${llvm_flags}>")
	else()
		target_compile_options(noder-compiler PUBLIC ${llvm_flags})
		target_link_libraries(noder-compiler PUBLIC ${llvm_libs})
	endif()
	
	set_target_properties(noder-compiler PROPERTIES FOLDER "Libs")
	if (isMultiConfig)
		install(TARGETS noder-compiler DESTINATION "$<CONFIG>/lib")
	else()
		install(TARGETS noder-compiler DESTINATION ${CMAKE_BUILD_TYPE}/lib)
	endif()
endif()

#for later
#target_compile_definitions(noder-core-dynamic PRIVATE NODERCORE_DYNAMIC)
#target_compile_definitions(noder-core-dynamic PRIVATE NODERCORE_EXPORTS)

if (BUILD_TESTS)
	add_subdirectory(tests)
endif()