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

add_executable(core-tests ${TEST_SOURCES} ${TESTS})
target_link_libraries(core-tests PRIVATE Catch2::Catch2 noder-interpreter)

add_custom_command(TARGET core-tests POST_BUILD
	COMMAND core-tests [unit],[integration]
)

set_target_properties(core-tests PROPERTIES FOLDER "Tests")