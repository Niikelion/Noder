cmake_minimum_required(VERSION 3.9.2)

project(Noder VERSION 1.0.0 DESCRIPTION "Node toolkit for visual scripting." LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(core)
add_subdirectory(external)
if (BUILD_TOOLS)
	add_subdirectory(tools)
endif()