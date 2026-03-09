include_guard(GLOBAL)

include(CMakeParseArguments)

if(NOT NDS_TOOLCHAIN_VENDOR STREQUAL "blocks")
	return()
endif()

function(ndsh_configure_blocksds_dylib_demo)
	set(oneValueArgs MAIN_TARGET)
	cmake_parse_arguments(NDD "" "${oneValueArgs}" "" ${ARGN})

	if(NOT NDD_MAIN_TARGET)
		message(FATAL_ERROR "MAIN_TARGET is required")
	endif()

	if(NOT TARGET ${NDD_MAIN_TARGET})
		message(FATAL_ERROR "MAIN_TARGET '${NDD_MAIN_TARGET}' does not exist")
	endif()

	option(NDSH_BUILD_DYLIB_DEMO "Build sample DSL interdependency demo for the 'dylib' command" ON)
	if(NOT NDSH_BUILD_DYLIB_DEMO)
		return()
	endif()

	add_library(A STATIC ${CMAKE_SOURCE_DIR}/src/dylib_demo/libA.cpp)
	add_library(B STATIC ${CMAKE_SOURCE_DIR}/src/dylib_demo/libB.cpp)
	add_library(C STATIC ${CMAKE_SOURCE_DIR}/src/dylib_demo/libC.cpp)

	blocksds_create_dsl(
		B
		MAIN_TARGET ${NDD_MAIN_TARGET}
	)

	blocksds_create_dsl(
		C
		MAIN_TARGET ${NDD_MAIN_TARGET}
		# DEPENDENCY_TARGETS curl_dsl
	)

	blocksds_create_dsl(
		A
		MAIN_TARGET ${NDD_MAIN_TARGET}
		DEPENDENCY_ELFS ${B_ELF} ${C_ELF}
	)

	add_custom_target(
		ndsh-dylib-demo
		ALL
		DEPENDS A_dsl B_dsl C_dsl
	)

	message(STATUS "BlocksDS dynamic library interdependency demo enabled:")
	message(STATUS "  A: ${A_DSL}")
	message(STATUS "  B: ${B_DSL}")
	message(STATUS "  C: ${C_DSL}")

	foreach(_demo_dsl IN ITEMS A B C)
		execute_process(
			COMMAND ${CMAKE_COMMAND} -E create_symlink
				"${${_demo_dsl}_DSL}" # blocksds_create_dsl() returns ABSOLUTE paths to the generated DSLs!
				"${CMAKE_SOURCE_DIR}/melonds-sdcard/${_demo_dsl}.dsl"
			COMMAND_ERROR_IS_FATAL ANY
		)
	endforeach()
endfunction()
