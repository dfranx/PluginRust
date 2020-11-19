cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17)
project(PluginRust)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ./bin)

# source code
set(SOURCES
	dllmain.cpp

# libraries
	libs/imgui/imgui_draw.cpp
	libs/imgui/imgui_widgets.cpp
	libs/imgui/imgui.cpp
)

# cmake toolchain 
if(CMAKE_TOOLCHAIN_FILE)
    include(${CMAKE_TOOLCHAIN_FILE})
endif(CMAKE_TOOLCHAIN_FILE)

# create executable
add_library(PluginRust SHARED ${SOURCES})

set_target_properties(PluginRust PROPERTIES OUTPUT_NAME "plugin")
set_target_properties(PluginRust PROPERTIES PREFIX "")

target_include_directories(PluginRust PRIVATE libs inc)

if (NOT MSVC)
	target_compile_options(GIFCapture PRIVATE -Wno-narrowing)
endif()