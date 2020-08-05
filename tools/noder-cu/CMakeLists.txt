set(NODER_CU_SOURCES
	src/main.cpp
)

add_executable(noder-cu ${NODER_CU_SOURCES})

target_link_libraries(noder-cu PRIVATE noder-core)

install(TARGETS noder-cu DESTINATION bin)

add_custom_command(TARGET noder-cu POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	$<TARGET_FILE:noder-core>
	$<TARGET_FILE_DIR:noder-cu>
)

set_target_properties(noder-cu ${PROJECT_BIN} PROPERTIES VC_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/${Configuration}")