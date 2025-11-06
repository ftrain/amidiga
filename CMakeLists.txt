cmake_minimum_required(VERSION 3.15)
project(gruvbok VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(Lua REQUIRED)

# RtMidi - try to find it or build from source
find_path(RTMIDI_INCLUDE_DIR RtMidi.h
    PATHS /usr/local/include /usr/include /opt/homebrew/include
)

find_library(RTMIDI_LIBRARY
    NAMES rtmidi
    PATHS /usr/local/lib /usr/lib /opt/homebrew/lib
)

if(RTMIDI_INCLUDE_DIR AND RTMIDI_LIBRARY)
    message(STATUS "Found RtMidi: ${RTMIDI_LIBRARY}")
    set(RTMIDI_FOUND TRUE)
else()
    message(STATUS "RtMidi not found system-wide, will need to be installed")
    set(RTMIDI_FOUND FALSE)
endif()

# Add subdirectories
add_subdirectory(src/core)
add_subdirectory(src/hardware)
add_subdirectory(src/lua_bridge)
add_subdirectory(src/desktop)

# Print configuration
message(STATUS "=== GRUVBOK Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Lua include: ${LUA_INCLUDE_DIR}")
message(STATUS "Lua library: ${LUA_LIBRARIES}")
if(RTMIDI_FOUND)
    message(STATUS "RtMidi include: ${RTMIDI_INCLUDE_DIR}")
    message(STATUS "RtMidi library: ${RTMIDI_LIBRARY}")
endif()
message(STATUS "=============================")
