set(INTER_SOURCES
	src/Interpreter.cpp

	include/Interpreter.hpp
)

set(MOD_SOURCES
	src/BaseModule.cpp
)

add_library(cu-interpreter ${INTER_SOURCES})
set_target_properties(cu-interpreter PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_include_directories(cu-interpreter PUBLIC include)
target_link_libraries(cu-interpreter PUBLIC noder-interpreter slib)
set_target_properties(cu-interpreter PROPERTIES FOLDER "Tools/noder-cu")

add_library(cu-interpreter-base SHARED ${MOD_SOURCES})
set_target_properties(cu-interpreter-base PROPERTIES PREFIX "")
target_link_libraries(cu-interpreter-base LINK_PRIVATE cu-interpreter)
set_target_properties(cu-interpreter-base PROPERTIES FOLDER "Tools/noder-cu")

if (BUILD_TESTS)
	add_subdirectory(tests)
endif()