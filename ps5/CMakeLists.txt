# PS5 Vulkan Template - the title's build (ps5/tools/build.sh drives it).
#
# Configured with the payload SDK's toolchain (toolchain/prospero.cmake), as
# PS5_Vulkan builds the CTS for the console. It compiles the base class, the
# samples listed in src/samples.cpp (each in a namespace of its own), the
# launcher and the third-party code into objects, then tools/link-title.sh
# links them with RADV and packages the title folder.
#
# Copyright (C) 2026 Mihawk
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20)
project(ps5_vulkan_samples C CXX)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
# The title's name, from its identity: klog's prefix and the menu's heading
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/sce_sys/param.json param_json)
string(JSON PS5_TITLE_NAME GET "${param_json}" localizedParameters en-US titleName)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS sce_sys/param.json)
set(PS5_VULKAN_DIR "${ROOT}/../PS5_Vulkan" CACHE PATH "the PS5_Vulkan checkout (RADV archive, link recipe, packaging)")
# PS5_PAYLOAD_SDK: the SDK the toolchain file came from (tools/setup-sdk.sh's pin)
# The host reference build: the same code on the PC's Vulkan driver, with a
# headless surface, reading and writing in PS5_HOST_APP_ROOT (tools/host-reference.sh)
option(PS5_HOST_REFERENCE "build for the PC, as a reference for the console's pictures" OFF)
set(PS5_HOST_APP_ROOT "${CMAKE_BINARY_DIR}/app" CACHE PATH "the host reference build's title folder")

# The samples: one SAMPLE(id, ...) line each in src/samples.cpp
file(READ src/samples.cpp samples_cpp)
string(REGEX MATCHALL "\n[ \t]*SAMPLE\\([a-z0-9_]+," sample_entries "${samples_cpp}")
set(PS5_SAMPLES)
foreach(entry IN LISTS sample_entries)
	string(REGEX REPLACE ".*SAMPLE\\(([a-z0-9_]+),$" "\\1" id "${entry}")
	list(APPEND PS5_SAMPLES ${id})
endforeach()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS src/samples.cpp)
message(STATUS "PS5 samples: ${PS5_SAMPLES}")

set(sample_sources)
foreach(id IN LISTS PS5_SAMPLES)
	set(ID ${id})
	set(SOURCE ${ROOT}/examples/${id}/${id}.cpp)
	if(EXISTS ${ROOT}/examples/${id}/main.cpp)
		set(SOURCE ${ROOT}/examples/${id}/main.cpp)
	endif()
	configure_file(src/wrap.cpp.in ${CMAKE_BINARY_DIR}/wrap/${id}.cpp @ONLY)
	list(APPEND sample_sources ${CMAKE_BINARY_DIR}/wrap/${id}.cpp)
endforeach()
file(WRITE ${CMAKE_BINARY_DIR}/samples.txt "${PS5_SAMPLES}")

file(GLOB base_sources ${ROOT}/base/*.cpp)
list(FILTER base_sources EXCLUDE REGEX "VulkanAndroid|VulkanOHOS")
set(third_party_sources
	${ROOT}/external/imgui/imgui.cpp
	${ROOT}/external/imgui/imgui_demo.cpp
	${ROOT}/external/imgui/imgui_draw.cpp
	${ROOT}/external/imgui/imgui_widgets.cpp
	# libktx reads the samples' .ktx files; its Vulkan calls (vkloader.c) go
	# through volk like the rest
	${ROOT}/external/ktx/lib/texture.c
	${ROOT}/external/ktx/lib/hashlist.c
	${ROOT}/external/ktx/lib/checkheader.c
	${ROOT}/external/ktx/lib/swap.c
	${ROOT}/external/ktx/lib/memstream.c
	${ROOT}/external/ktx/lib/filestream.c
	${ROOT}/external/ktx/lib/vkloader.c
	third_party/volk/volk.c)
set_source_files_properties(${third_party_sources} PROPERTIES COMPILE_OPTIONS "-w")

if(PS5_HOST_REFERENCE)
	set(platform_source src/platform_host.c)
	set(app_root ${PS5_HOST_APP_ROOT})
	set(surface_platform VK_USE_PLATFORM_HEADLESS_EXT PS5_HOST_REFERENCE "PS5_APP_ROOT=\"${PS5_HOST_APP_ROOT}\"")
else()
	set(platform_source src/platform.c)
	set(app_root /app0)
	set(surface_platform _DIRECT2DISPLAY)
endif()
add_library(samples OBJECT
	${base_sources}
	${third_party_sources}
	${sample_sources}
	src/main.cpp
	src/launcher.cpp
	src/samples.cpp
	src/example_ps5.cpp
	${platform_source})
target_compile_definitions(samples PRIVATE
	VK_EXAMPLE_PS5
	"PS5_TITLE_NAME=\"${PS5_TITLE_NAME}\""
	${surface_platform}
	VK_NO_PROTOTYPES
	NDEBUG
	VK_EXAMPLE_ASSETS_DIR="${app_root}/assets/"
	VK_EXAMPLE_SHADERS_DIR="${app_root}/shaders/")
target_include_directories(samples PRIVATE
	${ROOT}/external
	${ROOT}/external/glm
	${ROOT}/external/imgui
	${ROOT}/external/tinygltf
	${ROOT}/external/ktx/include
	${ROOT}/external/ktx/other_include
	${ROOT}/base
	third_party/volk
	src)
set_target_properties(samples PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON C_STANDARD 11)
# Every unit sees volk's declarations first: the Vulkan commands are volk's
# pointers, filled from the RADV linked into the title.
# -fno-omit-frame-pointer: the console's crash backtraces are frame-pointer walks.
target_compile_options(samples PRIVATE
	-O2 -g1 -ffunction-sections -fdata-sections -fno-omit-frame-pointer
	"SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/third_party/volk/volk.h")

# The UI module, when the title has it: PS5_VKHomebrewUI's kit, GPL-3.0-or-later
# (ui/README.md). Without ps5/ui/ nothing of it is built.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ui/ui.cmake)
	include(ui/ui.cmake)
else()
	file(REMOVE ${CMAKE_BINARY_DIR}/ui.txt)
endif()

if(PS5_HOST_REFERENCE)
	add_executable(ps5-samples-host $<TARGET_OBJECTS:samples>)
	target_link_libraries(ps5-samples-host PRIVATE dl pthread)
	return()
endif()

# Link, sign and package: the title folder in dist/<TITLE_ID>/
add_custom_command(
	OUTPUT ${CMAKE_BINARY_DIR}/eboot.elf
	COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tools/link-title.sh ${CMAKE_BINARY_DIR} ${PS5_VULKAN_DIR} ${PS5_PAYLOAD_SDK} "$<TARGET_OBJECTS:samples>"
	DEPENDS samples "$<TARGET_OBJECTS:samples>" ${CMAKE_CURRENT_SOURCE_DIR}/tools/link-title.sh
	COMMAND_EXPAND_LISTS
	VERBATIM)
add_custom_target(title ALL DEPENDS ${CMAKE_BINARY_DIR}/eboot.elf)
