include_guard(GLOBAL)

include(CMakeParseArguments)

if(NOT NDS_TOOLCHAIN_VENDOR STREQUAL "blocks")
	return()
endif()

include(BlocksDSDynamicLibraries)

function(ndsh_configure_blocksds_dylib_demo)
	set(options)
	set(oneValueArgs MAIN_TARGET)
	set(multiValueArgs)
	cmake_parse_arguments(NDD "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	if(NOT NDD_MAIN_TARGET)
		message(FATAL_ERROR "ndsh_configure_blocksds_dylib_demo(): MAIN_TARGET is required")
	endif()

	if(NOT TARGET ${NDD_MAIN_TARGET})
		message(FATAL_ERROR "ndsh_configure_blocksds_dylib_demo(): MAIN_TARGET '${NDD_MAIN_TARGET}' does not exist")
	endif()

	option(NDSH_BUILD_DYLIB_DEMO "Build sample DSL interdependency demo for the 'dylib' command" ON)
	if(NOT NDSH_BUILD_DYLIB_DEMO)
		return()
	endif()

	ndsh_add_blocksds_dsl_library(
		TARGET ndsh_dylib_libB
		OUTPUT_NAME libB
		SOURCES ${CMAKE_SOURCE_DIR}/src/dylib_demo/libB.cpp
		MAIN_TARGET ${NDD_MAIN_TARGET}
	)

	ndsh_add_blocksds_dsl_library(
		TARGET ndsh_dylib_libC
		OUTPUT_NAME libC
		SOURCES ${CMAKE_SOURCE_DIR}/src/dylib_demo/libC.cpp
		MAIN_TARGET ${NDD_MAIN_TARGET}
	)

	ndsh_add_blocksds_dsl_library(
		TARGET ndsh_dylib_libA
		OUTPUT_NAME libA
		SOURCES ${CMAKE_SOURCE_DIR}/src/dylib_demo/libA.cpp
		MAIN_TARGET ${NDD_MAIN_TARGET}
		DEPENDENCY_ELFS
			${ndsh_dylib_libB_ELF}
			${ndsh_dylib_libC_ELF}
	)

	add_custom_target(ndsh-dylib-demo ALL
		DEPENDS
			${ndsh_dylib_libA_DSL_TARGET}
			${ndsh_dylib_libB_DSL_TARGET}
			${ndsh_dylib_libC_DSL_TARGET}
	)

	message(STATUS "BlocksDS dynamic library interdependency demo enabled:")
	message(STATUS "  libA: ${ndsh_dylib_libA_DSL}")
	message(STATUS "  libB: ${ndsh_dylib_libB_DSL}")
	message(STATUS "  libC: ${ndsh_dylib_libC_DSL}")

	foreach(_demo_dsl IN ITEMS libA.dsl libB.dsl libC.dsl)
		execute_process(
			COMMAND ${CMAKE_COMMAND} -E create_symlink
				"${CMAKE_BINARY_DIR}/${_demo_dsl}"
				"${CMAKE_SOURCE_DIR}/melonds-sdcard/${_demo_dsl}"
			COMMAND_ERROR_IS_FATAL ANY
		)
	endforeach()
endfunction()
