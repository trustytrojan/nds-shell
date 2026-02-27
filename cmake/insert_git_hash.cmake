find_package(Git QUIET)
if(GIT_FOUND)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		OUTPUT_VARIABLE GIT_HASH
		OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_QUIET
	)
endif()

if(NOT GIT_HASH)
	set(GIT_HASH "unknown")
endif()

configure_file(
	"${PROJECT_SOURCE_DIR}/src/version.h.in"
	"${PROJECT_BINARY_DIR}/version.h"
)
