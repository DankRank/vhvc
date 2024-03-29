cmake_minimum_required(VERSION 3.8)
project(vhvc)
find_package(SDL2 REQUIRED)
add_library(imgui INTERFACE IMPORTED)
set_target_properties(imgui PROPERTIES
	INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/extlib/imgui;${CMAKE_CURRENT_SOURCE_DIR}/extlib/imgui/backends"
)
add_executable(vhvc
	src/common.hh
	src/main.cc
	src/imgui_compile.cc
	src/cpu.cc src/cpu.hh
	src/ppu.cc src/ppu.hh
	src/bus.cc src/bus.hh
	src/cpudebug.cc src/cpudebug.hh
	src/ppudebug.cc src/ppudebug.hh
	src/input.cc src/input.hh
	src/mapper.cc src/mapper.hh
	src/mapper_discrete.cc
	src/mapper_mmc.cc
	src/mapper_vrc.cc
	src/mapper_jaleco.cc
	src/mapper_namcot108.cc
	src/mapper_nsf.cc
	src/mapper_fds.cc
	src/nesfile.cc src/nesfile.hh
	src/nsffile.cc src/nsffile.hh
	src/fdsfile.cc src/fdsfile.hh
	src/span.hh
	src/io.hh
	src/palette.cc src/palette.hh
	src/apu.cc src/apu.hh
	src/audio.cc src/audio.hh
	src/state.hh
)
target_link_libraries(vhvc PRIVATE SDL2::SDL2 SDL2::SDL2main imgui)
target_compile_features(vhvc PRIVATE cxx_std_17)
if(MSVC)
	target_compile_options(vhvc PRIVATE /Zc:__cplusplus /D_CRT_SECURE_NO_WARNINGS /W3 /wd4267 /wd4244 /utf-8)
else()
	target_compile_options(vhvc PRIVATE -Wall -Wextra -Wno-parentheses)
endif()
