# PS5 Vulkan Template - the UI module: PS5_VKHomebrewUI's kit in the title's build.
#
# ps5/CMakeLists.txt includes this file when ps5/ui/ is present (a title made
# with new-title.py --ui, and PS5 Vulkan Samples). It exports the pinned kit
# (setup-kit.sh), compiles its code into the samples' objects, minus what is
# OpenGL's or the kit's own console layer (the template has its own), and
# writes build/ps5/ui.txt, which tells build-assets.py to lay the kit's fonts
# and sounds in.
#
# The kit and this folder are GPL-3.0-or-later (ps5/ui/README.md); a title
# without ps5/ui/ builds none of it.
#
# Copyright (C) 2026 Mihawk
# SPDX-License-Identifier: GPL-3.0-or-later

execute_process(
	COMMAND bash ${CMAKE_CURRENT_LIST_DIR}/setup-kit.sh
	OUTPUT_VARIABLE PS5_UI_KIT
	OUTPUT_STRIP_TRAILING_WHITESPACE
	RESULT_VARIABLE ui_kit_result)
if(NOT ui_kit_result EQUAL 0 OR NOT EXISTS ${PS5_UI_KIT}/src/gfx/vk/vk_renderer.cpp)
	message(FATAL_ERROR "ps5/ui/setup-kit.sh could not export the pinned PS5_VKHomebrewUI")
endif()
# A new pin in setup-kit.sh configures the build again
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
	${CMAKE_CURRENT_LIST_DIR}/setup-kit.sh ${PS5_UI_KIT}/.revision)
message(STATUS "PS5 UI kit: ${PS5_UI_KIT}")

file(GLOB_RECURSE ui_kit_sources ${PS5_UI_KIT}/src/*.cpp ${PS5_UI_KIT}/src/*.c)
# Not the kit's console entry point and platform layer (the template's
# platform.c and hui_platform.cpp stand in), not its OpenGL backend
list(FILTER ui_kit_sources EXCLUDE REGEX "/src/(main\\.cpp|platform/|runtime/)")
list(FILTER ui_kit_sources EXCLUDE REGEX "/src/gfx/(gl_[a-z_]+|backdrop|canvas)\\.cpp$")
set_source_files_properties(${ui_kit_sources} PROPERTIES COMPILE_OPTIONS "-w")

# A program's own kit code (examples/<id>/kit/*.cpp: a design copied into the
# title by new-title.py --ui) is compiled as it stands, outside the namespace
# a program's main file is wrapped in, since it opens the kit's own namespaces
set(ui_program_sources)
foreach(id IN LISTS PS5_SAMPLES)
	file(GLOB program_kit_sources ${ROOT}/examples/${id}/kit/*.cpp)
	list(APPEND ui_program_sources ${program_kit_sources})
endforeach()

target_sources(samples PRIVATE
	${ui_kit_sources}
	${ui_program_sources}
	${CMAKE_CURRENT_LIST_DIR}/kit.cpp
	${CMAKE_CURRENT_LIST_DIR}/hui_platform.cpp)
target_include_directories(samples PRIVATE ${PS5_UI_KIT}/src ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(samples PRIVATE PS5_UI)
file(WRITE ${CMAKE_BINARY_DIR}/ui.txt "${PS5_UI_KIT}\n")
