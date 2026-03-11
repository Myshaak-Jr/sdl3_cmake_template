cmake_minimum_required(VERSION 3.20)

set(MAIN_TARGET "my-app")
project(${MAIN_TARGET} VERSION 0.1.0 LANGUAGES C CXX)

add_executable(${MAIN_TARGET})
target_compile_features(${MAIN_TARGET} PRIVATE cxx_std_23)

target_sources(${MAIN_TARGET} PRIVATE
	"src/main.c"
	"src/shaders.c"
)

target_include_directories(${MAIN_TARGET} PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	"${CMAKE_CURRENT_BINARY_DIR}/generated"
)

# ==== Shaders sources ====
set(SHADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")

set(SHADER_INPUTS  # list files
	"${SHADERS_DIR}/position_color.vert.hlsl"
	"${SHADERS_DIR}/color.frag.hlsl"
)

# ==== SDL3 from vcpkg ====
find_package(SDL3 CONFIG REQUIRED)
target_link_libraries(${MAIN_TARGET} PRIVATE SDL3::SDL3)

# ==== Shader compilation ====
option(SHADERS_BUILD_SPIRV "Build SPIR-V shaders" ON)
option(SHADERS_BUILD_DXIL  "Build DXIL shaders"  OFF)
option(SHADERS_BUILD_MSL   "Build MSL shaders"   OFF)

set(SHADERS_DIR_SPIRV "SPIRV")
set(SHADERS_DIR_DXIL  "DXIL")
set(SHADERS_DIR_MSL   "MSL")

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/shader_config.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/generated/shader_config.h"
)

set(SHADER_DESTS "")
if(SHADERS_BUILD_SPIRV)
	list(APPEND SHADER_DESTS ${SHADERS_DIR_SPIRV})
endif()
if(SHADERS_BUILD_DXIL)
	list(APPEND SHADER_DESTS ${SHADERS_DIR_DXIL})
endif()
if(SHADERS_BUILD_MSL)
	list(APPEND SHADER_DESTS ${SHADERS_DIR_MSL})
endif()

if(SHADER_DESTS STREQUAL "")
	message(FATAL_ERROR "No shader destinations enabled. Enable at least one of SHADERS_BUILD_*.")
endif()

# Find the shadercross CLI executable.
# The CLI is named "shadercross" in SDL_shadercross docs.

set(_shadercross_hints "")
if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
	list(APPEND _shadercross_hints
		"${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/sdl3-shadercross"
		"${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/SDL3_shadercross"
	)
endif()

find_program(SDL_SHADERCROSS_EXE
	NAMES shadercross shadercross.exe
	HINTS ${_shadercross_hints}
	REQUIRED
)

set(SHADERS_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")

# Map filename to stage for safety.
# Expected patterns: *.vert.*  *.frag.*  *.comp.*
function(_shader_stage_from_name in_file out_stage)
	get_filename_component(_name "${in_file}" NAME)
	string(TOLOWER "${_name}" _lname)

	if(_lname MATCHES "\\.vert\\.")
		set(${out_stage} "vertex" PARENT_SCOPE)
	elseif(_lname MATCHES "\\.frag\\.")
		set(${out_stage} "fragment" PARENT_SCOPE)
	elseif(_lname MATCHES "\\.comp\\.")
		set(${out_stage} "compute" PARENT_SCOPE)
	else()
		message(FATAL_ERROR
			"Can't infer shader stage from filename '${_name}'. "
			"Use .vert., .frag., or .comp. in the name (e.g. foo.vert.hlsl)."
		)
	endif()
endfunction()


set(SHADER_OUTPUTS "")


foreach(SHADER_IN IN LISTS SHADER_INPUTS)
	get_filename_component(_name "${SHADER_IN}" NAME)
	string(REGEX REPLACE "\\.hlsl$" "" BASE "${_name}")

	_shader_stage_from_name("${SHADER_IN}" STAGE)

	foreach (DEST IN LISTS SHADER_DESTS)
		set(OUT_DIR "${SHADERS_OUT_DIR}/${DEST}")

		if (DEST STREQUAL "SPIRV")
			set(OUT "${OUT_DIR}/${BASE}.spv")
		elseif (DEST STREQUAL "DXIL")
			set(OUT "${OUT_DIR}/${BASE}.dxil")
		elseif (DEST STREQUAL "MSL")
			set(OUT "${OUT_DIR}/${BASE}.msl")
		endif()

		add_custom_command(
			OUTPUT ${OUT}

			COMMAND ${CMAKE_COMMAND} -E make_directory "${OUT_DIR}"
			COMMAND "${SDL_SHADERCROSS_EXE}" "${SHADER_IN}" -t "${STAGE}" -d "${DEST}" -o "${OUT}"

			DEPENDS "${SHADER_IN}"
			VERBATIM
		)

		list(APPEND SHADER_OUTPUTS "${OUT}")

	endforeach()
endforeach()

add_custom_target(compile_shaders ALL
	DEPENDS ${SHADER_OUTPUTS}
)

# Ensure shaders are built before the executable if you want that coupling:
add_dependencies(${MAIN_TARGET} compile_shaders)

# Export compile commands to root
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
    )
endif()
