set(LLVM_CONFIG   "" CACHE FILEPATH "Path to llvm-config")
set(LLVM_CONFIG_D "" CACHE FILEPATH "Path to llvm-config(debug)")

if (LLVM_CONFIG STREQUAL "")
	message(FATAL_ERROR "You must specify llvm-config path")
endif()

if (LLVM_CONFIG_D STREQUAL "")
	message(FATAL_ERROR "You must specify llvm-config path for debug configuration")
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
	--link-static --libnames codegen interpreter executionengine linker mcjit mcparser native tablegen
	OUTPUT_VARIABLE llvm_libs OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(llvm_libs)

execute_process(COMMAND ${LLVM_CONFIG}
	--libdir
	OUTPUT_VARIABLE llvm_lib_dir OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND ${LLVM_CONFIG_D}
	--libdir
	OUTPUT_VARIABLE llvm_lib_dir_d OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND ${LLVM_CONFIG}
	--cppflags
	OUTPUT_VARIABLE llvm_flags OUTPUT_STRIP_TRAILING_WHITESPACE)

separate_arguments(llvm_flags)

execute_process(COMMAND ${LLVM_CONFIG_D}
	--cppflags
	OUTPUT_VARIABLE llvm_flags_d OUTPUT_STRIP_TRAILING_WHITESPACE)

separate_arguments(llvm_flags_d)

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

add_library(noder-core SHARED ${sources} ${headers})
target_include_directories(noder-core PRIVATE ${LLVM_INCLUDE} ${llvm_include_dir} ${llvm_root}/include)
target_include_directories(noder-core PUBLIC include)

target_link_libraries(noder-core LINK_PRIVATE ${sys_libs} ${llvm_libs})
target_link_directories(noder-core PRIVATE "$<$<CONFIG:Debug>:${llvm_lib_dir_d}>$<$<NOT:$<CONFIG:Debug>>:${llvm_lib_dir}>")
target_compile_options(noder-core PRIVATE "$<$<CONFIG:Debug>:${llvm_flags_d}>$<$<NOT:$<CONFIG:Debug>>:${llvm_flags}>")
target_compile_definitions(noder-core PRIVATE nodercore_EXPORTS)

install(TARGETS noder-core DESTINATION lib)
install(FILES ${headers} DESTINATION include)

set(BUILD_TESTS false CACHE BOOL "Build core tests")

if (BUILD_TESTS)
	add_subdirectory(tests)
endif()