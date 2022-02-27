cmake_minimum_required(VERSION 3.18)
project(vhvc)
find_package(SDL2 REQUIRED)
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
	src/nesfile.cc src/nesfile.hh
	src/span.hh
	src/io.hh
	src/palette.cc src/palette.hh
	src/apu.cc src/apu.hh
	src/audio.cc src/audio.hh
)
target_include_directories(vhvc PRIVATE ${SDL2_INCLUDE_DIRS} extlib/imgui extlib/imgui/backends)
target_link_libraries(vhvc PRIVATE ${SDL2_LIBRARIES})
target_compile_features(vhvc PRIVATE cxx_std_17)
if(MSVC)
	target_compile_options(vhvc PRIVATE "/Zc:__cplusplus")
endif()
