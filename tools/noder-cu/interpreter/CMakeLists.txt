set(INTER_SOURCES
	src/Interpreter.cpp

	include/Interpreter.hpp
)

set(MOD_SOURCES
	src/BaseModule.cpp
)

add_library(cu-interpreter ${INTER_SOURCES})
target_link_libraries(cu-interpreter PUBLIC noder-interpreter)
target_include_directories(cu-interpreter PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/include)
set_target_properties(cu-interpreter PROPERTIES FOLDER "Tools/noder-cu")

add_library(cu-interpreter-base SHARED ${MOD_SOURCES})
target_link_libraries(cu-interpreter-base LINK_PRIVATE cu-interpreter)
set_target_properties(cu-interpreter-base PROPERTIES FOLDER "Tools/noder-cu")

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/include/config.hpp.in
	${CMAKE_CURRENT_BINARY_DIR}/include/config.hpp
)

if (BUILD_TESTS)
	add_subdirectory(tests)
endif()