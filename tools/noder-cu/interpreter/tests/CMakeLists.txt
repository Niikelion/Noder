find_package(Catch2 REQUIRED)

#function to append extensions
FUNCTION(APPEND_EXT var source ext)
	SET(listVar "")
	FOREACH(f ${${source}})
		LIST(APPEND listVar "${f}.${ext}")
	ENDFOREACH(f)
	SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(APPEND_EXT)

set(TEST_SOURCES
	main.cpp
)

set(TESTS
	unit
)

APPEND_EXT(TESTS TESTS "cpp")

add_executable(cu-interpreter-tests ${TEST_SOURCES} ${TESTS})
target_link_libraries(cu-interpreter-tests PRIVATE Catch2::Catch2 cu-interpreter)

add_custom_command(TARGET cu-interpreter-tests POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$<TARGET_FILE_DIR:cu-interpreter-base>
		$<TARGET_FILE_DIR:cu-interpreter-tests>
	COMMAND cu-interpreter-tests [unit],[integration]
)

set_target_properties(cu-interpreter-tests PROPERTIES FOLDER "Tests")