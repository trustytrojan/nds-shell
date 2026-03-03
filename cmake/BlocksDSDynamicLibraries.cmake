include_guard(GLOBAL)

include(CMakeParseArguments)

# BlocksDS dynamic library helper for nds-shell.
#
# Why this module exists:
# - CMake doesn't support custom add_library() types for DSL directly.
# - We model a DSL as: OBJECT library -> linked .elf -> converted .dsl via dsltool.
# - This keeps DSL creation declarative and reusable for multiple libraries.
#
# Main function:
#   ndsh_add_blocksds_dsl_library(
#     TARGET <logical_name>
#     SOURCES <src1> [src2...]
#     [OUTPUT_NAME <artifact_base_name>]
#     [INCLUDE_DIRECTORIES <dir1> [dir2...]]
#     [COMPILE_DEFINITIONS <def1> [def2...]]
#     [MAIN_ELF <path/to/main.elf>]
#   )
#
# Exposed parent-scope variables (for TARGET foo):
# - foo_ELF        : generated .elf path
# - foo_DSL        : generated .dsl path
# - foo_DSL_TARGET : custom target that builds the DSL
#
# Typical multi-DSL pattern in the caller:
#   ndsh_add_blocksds_dsl_library(TARGET plugin_a SOURCES a.cpp)
#   ndsh_add_blocksds_dsl_library(TARGET plugin_b SOURCES b.cpp)
#   add_custom_target(all-dsls ALL
#       DEPENDS ${plugin_a_DSL_TARGET} ${plugin_b_DSL_TARGET})

if(NOT NDS_TOOLCHAIN_VENDOR STREQUAL "blocks")
	return()
endif()

set(NDSH_DSLTOOL "$ENV{BLOCKSDS}/tools/dsltool/dsltool")
set(NDSH_DSL_SPECS "$ENV{BLOCKSDS}/sys/crts/ds_arm9_dsl.specs")

if(NOT EXISTS ${NDSH_DSLTOOL})
	message(FATAL_ERROR "dsltool not found at ${NDSH_DSLTOOL}")
endif()

if(NOT EXISTS ${NDSH_DSL_SPECS})
	message(FATAL_ERROR "BlocksDS DSL specs file not found at ${NDSH_DSL_SPECS}")
endif()

function(ndsh_add_blocksds_dsl_library)
	# Required:
	# - TARGET  : logical name used to create internal build targets
	# - SOURCES : source files for this dynamic library
	#
	# Optional:
	# - OUTPUT_NAME         : final artifact basename (default: TARGET)
	# - MAIN_ELF            : if provided, passed to dsltool -m for symbol resolution
	# - INCLUDE_DIRECTORIES : include paths for compiling the DSL sources
	# - COMPILE_DEFINITIONS : compile definitions for this DSL only
	set(options)
	set(oneValueArgs TARGET OUTPUT_NAME MAIN_ELF)
	set(multiValueArgs SOURCES INCLUDE_DIRECTORIES COMPILE_DEFINITIONS)
	cmake_parse_arguments(NDSL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	if(NOT NDSL_TARGET)
		message(FATAL_ERROR "ndsh_add_blocksds_dsl_library(): TARGET is required")
	endif()

	if(NOT NDSL_SOURCES)
		message(FATAL_ERROR "ndsh_add_blocksds_dsl_library(): SOURCES is required")
	endif()

	set(_basename "${NDSL_TARGET}")
	if(NDSL_OUTPUT_NAME)
		set(_basename "${NDSL_OUTPUT_NAME}")
	endif()

	set(_obj_target "${NDSL_TARGET}__obj")
	set(_dsl_target "${NDSL_TARGET}__dsl")
	set(_elf "${CMAKE_BINARY_DIR}/${_basename}.elf")
	set(_dsl "${CMAKE_BINARY_DIR}/${_basename}.dsl")

	# Compile to object files first; this makes per-DSL compile options easy and
	# lets us keep CMake target semantics while producing a non-native artifact.
	add_library(${_obj_target} OBJECT ${NDSL_SOURCES})

	target_compile_options(${_obj_target} PRIVATE
		-mthumb
		-mcpu=arm946e-s+nofp
		-O2
		-ffunction-sections
		-fdata-sections
		-fvisibility=hidden
	)

	if(NDSL_INCLUDE_DIRECTORIES)
		target_include_directories(${_obj_target} PRIVATE ${NDSL_INCLUDE_DIRECTORIES})
	endif()

	if(NDSL_COMPILE_DEFINITIONS)
		target_compile_definitions(${_obj_target} PRIVATE ${NDSL_COMPILE_DEFINITIONS})
	endif()

	# Link raw objects into a relocatable-friendly ARM9 ELF for dsltool.
	add_custom_command(
		OUTPUT ${_elf}
		COMMAND ${CMAKE_CXX_COMPILER}
			-mthumb
			-mcpu=arm946e-s+nofp
			-nostdlib
			-specs=${NDSH_DSL_SPECS}
			-Wl,--emit-relocs
			-Wl,--unresolved-symbols=ignore-all
			-Wl,--nmagic
			-Wl,--target1-abs
			$<TARGET_OBJECTS:${_obj_target}>
			-o ${_elf}
		DEPENDS ${_obj_target}
		VERBATIM
	)

	set(_dsltool_args -i ${_elf} -o ${_dsl})
	# MAIN_ELF is optional. Use it when you want dsltool to resolve symbols
	# against the main binary at build-time.
	if(NDSL_MAIN_ELF)
		list(APPEND _dsltool_args -m ${NDSL_MAIN_ELF})
	endif()

	add_custom_command(
		OUTPUT ${_dsl}
		COMMAND ${NDSH_DSLTOOL} ${_dsltool_args}
		DEPENDS ${_elf}
		VERBATIM
	)

	# Build target for this individual DSL.
	add_custom_target(${_dsl_target} DEPENDS ${_dsl})

	# Return convenient values to the caller.
	set(${NDSL_TARGET}_ELF ${_elf} PARENT_SCOPE)
	set(${NDSL_TARGET}_DSL ${_dsl} PARENT_SCOPE)
	set(${NDSL_TARGET}_DSL_TARGET ${_dsl_target} PARENT_SCOPE)
endfunction()
