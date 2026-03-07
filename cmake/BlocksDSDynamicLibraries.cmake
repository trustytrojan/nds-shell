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
#     STATIC_TARGET <existing_static_library_target>
#     [OUTPUT_NAME <artifact_base_name>]
#     [MAIN_TARGET <target_name>]
#     [DEPENDENCY_TARGETS <target1> [target2...]]
#   )
#
# Exposed parent-scope variables (for TARGET foo):
# - foo_ELF        : generated .elf path
# - foo_DSL        : generated .dsl path
# - foo_DSL_TARGET : custom target that builds the DSL
#
# Typical multi-DSL pattern in the caller:
#   ndsh_add_blocksds_dsl_library(TARGET plugin_a STATIC_TARGET plugin_a_static)
#   ndsh_add_blocksds_dsl_library(TARGET plugin_b STATIC_TARGET plugin_b_static)
#   add_custom_target(all-dsls ALL
#       DEPENDS ${plugin_a_DSL_TARGET} ${plugin_b_DSL_TARGET})

if(NOT NDS_TOOLCHAIN_VENDOR STREQUAL "blocks")
	return()
endif()

set(NDSH_DSLTOOL "$ENV{BLOCKSDS}/tools/dsltool/dsltool" CACHE STRING "")
set(NDSH_DSL_SPECS "$ENV{BLOCKSDS}/sys/crts/ds_arm9_dsl.specs" CACHE STRING "")

if(NOT EXISTS ${NDSH_DSLTOOL})
	message(FATAL_ERROR "dsltool not found at ${NDSH_DSLTOOL}")
endif()

if(NOT EXISTS ${NDSH_DSL_SPECS})
	message(FATAL_ERROR "BlocksDS DSL specs file not found at ${NDSH_DSL_SPECS}")
endif()

function(ndsh_add_blocksds_dsl_library)
	# Required:
	# - TARGET  : logical name used to create internal build targets
	# - STATIC_TARGET : existing static library target used as linker input for the DSL ELF
	#
	# Optional:
	# - OUTPUT_NAME         : final artifact basename (default: TARGET)
	# - MAIN_TARGET         : if provided, uses TARGET_FILE of this CMake target for dsltool -m
	# - DEPENDENCY_TARGETS  : target(s) passed to dsltool -d
	#                         If an entry names a CMake target, its TARGET_FILE is used.
	#                         If an entry names another ndsh_add_blocksds_dsl_library() logical TARGET,
	#                         that helper's <TARGET>_ELF output is used.
	set(options)
	set(oneValueArgs TARGET STATIC_TARGET OUTPUT_NAME MAIN_TARGET)
	set(multiValueArgs DEPENDENCY_TARGETS)
	cmake_parse_arguments(NDSL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	if(NOT NDSL_TARGET)
		message(FATAL_ERROR "ndsh_add_blocksds_dsl_library(): TARGET is required")
	endif()

	if(NOT NDSL_STATIC_TARGET)
		message(FATAL_ERROR "ndsh_add_blocksds_dsl_library(): STATIC_TARGET is required")
	endif()

	if(NOT TARGET ${NDSL_STATIC_TARGET})
		message(FATAL_ERROR "ndsh_add_blocksds_dsl_library(): STATIC_TARGET '${NDSL_STATIC_TARGET}' does not exist")
	endif()

	get_target_property(_static_target_type ${NDSL_STATIC_TARGET} TYPE)
	if(NOT _static_target_type STREQUAL "STATIC_LIBRARY")
		message(FATAL_ERROR "ndsh_add_blocksds_dsl_library(): STATIC_TARGET '${NDSL_STATIC_TARGET}' must be a STATIC library target")
	endif()

	set(_basename "${NDSL_TARGET}")
	if(NDSL_OUTPUT_NAME)
		set(_basename "${NDSL_OUTPUT_NAME}")
	endif()

	set(_dsl_target "${NDSL_TARGET}__dsl")
	set(_elf "${CMAKE_BINARY_DIR}/${_basename}.elf")
	set(_dsl "${CMAKE_BINARY_DIR}/${_basename}.dsl")

	# Link static library objects into a relocatable-friendly ARM9 ELF for dsltool.
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
			-Wl,--whole-archive
			$<TARGET_FILE:${NDSL_STATIC_TARGET}>
			-Wl,--no-whole-archive
			-o ${_elf}
		DEPENDS ${NDSL_STATIC_TARGET}
		VERBATIM
	)

	set(_dsltool_args -i ${_elf} -o ${_dsl})
	set(_dsltool_deps ${_elf})
	
	# MAIN_TARGET is optional.
	# Use it when you want dsltool to resolve symbols against the main binary at build-time.
	if(NDSL_MAIN_TARGET)
		if(NOT TARGET ${NDSL_MAIN_TARGET})
			message(FATAL_ERROR "ndsh_add_blocksds_dsl_library(): MAIN_TARGET '${NDSL_MAIN_TARGET}' does not exist")
		endif()

		list(APPEND _dsltool_args -m $<TARGET_FILE:${NDSL_MAIN_TARGET}>)
		list(APPEND _dsltool_deps ${NDSL_MAIN_TARGET})
	endif()

	foreach(_dep_target IN LISTS NDSL_DEPENDENCY_TARGETS)
		if(TARGET ${_dep_target})
			list(APPEND _dsltool_args -d $<TARGET_FILE:${_dep_target}>)
			list(APPEND _dsltool_deps ${_dep_target})
		elseif(DEFINED ${_dep_target}_ELF)
			list(APPEND _dsltool_args -d ${${_dep_target}_ELF})
			if(DEFINED ${_dep_target}_DSL_TARGET)
				list(APPEND _dsltool_deps ${${_dep_target}_DSL_TARGET})
			endif()
		else()
			get_property(_dep_has_elf GLOBAL PROPERTY NDSH_DSL_${_dep_target}_ELF SET)
			if(_dep_has_elf)
				get_property(_dep_elf GLOBAL PROPERTY NDSH_DSL_${_dep_target}_ELF)
				list(APPEND _dsltool_args -d ${_dep_elf})
				get_property(_dep_has_dsl_target GLOBAL PROPERTY NDSH_DSL_${_dep_target}_DSL_TARGET SET)
				if(_dep_has_dsl_target)
					get_property(_dep_dsl_target GLOBAL PROPERTY NDSH_DSL_${_dep_target}_DSL_TARGET)
					list(APPEND _dsltool_deps ${_dep_dsl_target})
				endif()
			else()
				message(FATAL_ERROR "ndsh_add_blocksds_dsl_library(): dependency '${_dep_target}' is neither a CMake target nor a known ndsh DSL logical target")
			endif()
		endif()
	endforeach()

	add_custom_command(
		OUTPUT ${_dsl}
		COMMAND ${NDSH_DSLTOOL} ${_dsltool_args}
		DEPENDS ${_dsltool_deps}
		VERBATIM
	)

	# Build target for this individual DSL.
	add_custom_target(${_dsl_target} DEPENDS ${_dsl})

	# Return convenient values to the caller.
	set(${NDSL_TARGET}_ELF ${_elf} PARENT_SCOPE)
	set(${NDSL_TARGET}_DSL ${_dsl} PARENT_SCOPE)
	set(${NDSL_TARGET}_DSL_TARGET ${_dsl_target} PARENT_SCOPE)

	# Also register globally so dependencies can be resolved across directories.
	set_property(GLOBAL PROPERTY NDSH_DSL_${NDSL_TARGET}_ELF ${_elf})
	set_property(GLOBAL PROPERTY NDSH_DSL_${NDSL_TARGET}_DSL ${_dsl})
	set_property(GLOBAL PROPERTY NDSH_DSL_${NDSL_TARGET}_DSL_TARGET ${_dsl_target})
endfunction()
