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

	add_library(ndsh_dylib_libB_static STATIC ${CMAKE_SOURCE_DIR}/src/dylib_demo/libB.cpp)
	add_library(ndsh_dylib_libC_static STATIC ${CMAKE_SOURCE_DIR}/src/dylib_demo/libC.cpp)
	add_library(ndsh_dylib_libA_static STATIC ${CMAKE_SOURCE_DIR}/src/dylib_demo/libA.cpp)

	ndsh_add_blocksds_dsl_library(
		TARGET ndsh_dylib_libB
		STATIC_TARGET ndsh_dylib_libB_static
		OUTPUT_NAME libB
		MAIN_TARGET ${NDD_MAIN_TARGET}
	)

	ndsh_add_blocksds_dsl_library(
		TARGET ndsh_dylib_libC
		STATIC_TARGET ndsh_dylib_libC_static
		OUTPUT_NAME libC
		MAIN_TARGET ${NDD_MAIN_TARGET}
		DEPENDENCY_TARGETS curl_dsl
	)

	ndsh_add_blocksds_dsl_library(
		TARGET ndsh_dylib_libA
		STATIC_TARGET ndsh_dylib_libA_static
		OUTPUT_NAME libA
		MAIN_TARGET ${NDD_MAIN_TARGET}
		DEPENDENCY_TARGETS
			ndsh_dylib_libB
			ndsh_dylib_libC
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
