
add_library(slib src/slib.cpp include/slib.hpp)
target_include_directories(slib PUBLIC include ${CMAKE_CURRENT_BINARY_DIR}/include)

if (WIN32)
	set(SYS_LIBS Kernel32)
endif()
if (UNIX)
	set(SYS_LIBS dl)
endif()

target_link_libraries(slib ${SYS_LIBS})

configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/include/config.hpp.in
	${CMAKE_CURRENT_BINARY_DIR}/include/config.hpp
)