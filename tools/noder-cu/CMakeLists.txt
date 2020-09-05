add_subdirectory(interpreter)

set(Python_USE_STATIC_LIBS TRUE)

FIND_PACKAGE(PythonInterp   3)
find_package(pybind11 REQUIRED)

set(NODER_CU_SOURCES
	src/main.cpp
	src/creator.cpp

	include/creator.hpp
)

add_executable(noder-cu ${NODER_CU_SOURCES})

target_include_directories(noder-cu PRIVATE include)
target_link_libraries(noder-cu PRIVATE cu-interpreter cxxopts termcolor nhash pybind11::embed pybind11::module pybind11::pybind11)

set_property(TARGET noder-cu PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:noder-cu>")
set_target_properties(noder-cu PROPERTIES FOLDER "Tools/noder-cu")

set(NODER_PYTHON_PATH ${PYTHON_EXECUTABLE})
get_filename_component(NODER_PYTHON_PATH ${NODER_PYTHON_PATH} DIRECTORY)

add_custom_command(TARGET noder-cu POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE_DIR:cu-interpreter-base>
		$<TARGET_FILE_DIR:noder-cu>
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${NODER_PYTHON_PATH}/${CMAKE_SHARED_LIBRARY_PREFIX}python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}${CMAKE_SHARED_LIBRARY_SUFFIX}
		$<TARGET_FILE_DIR:noder-cu>
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${NODER_PYTHON_PATH}/Lib
		$<TARGET_FILE_DIR:noder-cu>/Lib
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${NODER_PYTHON_PATH}/DLLs
		$<TARGET_FILE_DIR:noder-cu>/DLLs
)