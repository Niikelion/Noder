set(LLVM_CONFIG   "" CACHE FILEPATH "Path to llvm-config")
set(LLVM_CONFIG_D "" CACHE FILEPATH "Path to llvm-config(debug)")
set(BUILD_TESTS false CACHE BOOL "Build core tests")

find_package(parselib REQUIRED)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.9)
         get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
elseif(CMAKE_CONFIGURATION_TYPES)
         set(isMultiConfig TRUE)
endif()

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
	--link-static --libfiles codegen interpreter executionengine linker mcjit mcparser native tablegen
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
		--link-static --libfiles codegen interpreter executionengine linker mcjit mcparser native tablegen
		OUTPUT_VARIABLE llvm_libs_d OUTPUT_STRIP_TRAILING_WHITESPACE)
	separate_arguments(llvm_libs_d)

	execute_process(COMMAND ${LLVM_CONFIG_D}
		--cppflags
		OUTPUT_VARIABLE llvm_flags_d OUTPUT_STRIP_TRAILING_WHITESPACE)

	separate_arguments(llvm_flags_d)
endif()

set(sources
	src/Interpreter.cpp
	src/NodeCore.cpp
	src/Nodes.cpp
)

set(headers
	include/noder/Interpreter.hpp
	include/noder/NodeCore.hpp
	include/noder/Nodes.hpp
	include/noder/NodeUtils.hpp
)

#dynamic library
add_library(noder-core-dynamic SHARED ${sources} ${headers})
target_link_libraries(noder-core-dynamic PRIVATE parselib::xml parselib::json)
target_include_directories(noder-core-dynamic PRIVATE ${LLVM_INCLUDE} ${llvm_include_dir} ${llvm_root}/include)
target_include_directories(noder-core-dynamic PUBLIC include)

target_link_libraries(noder-core-dynamic PUBLIC ${sys_libs})
if (isMultiConfig)
	target_link_libraries(noder-core-dynamic LINK_PRIVATE "$<$<CONFIG:Debug>:${llvm_libs_d}>$<$<NOT:$<CONFIG:Debug>>:${llvm_libs}>")
	target_compile_options(noder-core-dynamic PRIVATE "$<$<CONFIG:Debug>:${llvm_flags_d}>$<$<NOT:$<CONFIG:Debug>>:${llvm_flags}>")
else()
	target_compile_options(noder-core-dynamic PRIVATE ${llvm_flags})
	target_link_libraries(noder-core-dynamic LINK_PRIVATE ${llvm_libs})
endif()

target_compile_definitions(noder-core-dynamic PRIVATE NODERCORE_DYNAMIC)
target_compile_definitions(noder-core-dynamic PRIVATE NODERCORE_EXPORTS)

install(TARGETS noder-core-dynamic DESTINATION lib)

#static library
add_library(noder-core ${sources} ${headers})
target_link_libraries(noder-core PUBLIC parselib::xml parselib::json)
target_include_directories(noder-core PRIVATE ${LLVM_INCLUDE} ${llvm_include_dir} ${llvm_root}/include)
target_include_directories(noder-core PUBLIC include)

target_link_libraries(noder-core PUBLIC ${sys_libs})
if (isMultiConfig)
	target_link_libraries(noder-core PUBLIC "$<$<CONFIG:Debug>:${llvm_libs_d}>$<$<NOT:$<CONFIG:Debug>>:${llvm_libs}>")
	target_compile_options(noder-core PUBLIC "$<$<CONFIG:Debug>:${llvm_flags_d}>$<$<NOT:$<CONFIG:Debug>>:${llvm_flags}>")
else()
	target_compile_options(noder-core PUBLIC ${llvm_flags})
	target_link_libraries(noder-core PUBLIC ${llvm_libs})
endif()

install(TARGETS noder-core DESTINATION lib)

#headers
install(FILES ${headers} DESTINATION include)

if (BUILD_TESTS)
	add_subdirectory(tests)
endif()