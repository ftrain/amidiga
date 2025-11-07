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

# Find FluidSynth (optional)
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(FLUIDSYNTH fluidsynth)
endif()

# If pkg-config didn't find it, try Homebrew paths on macOS
if(NOT FLUIDSYNTH_FOUND AND APPLE)
    find_library(FLUIDSYNTH_LIBRARY
        NAMES fluidsynth
        PATHS /opt/homebrew/lib /usr/local/lib
        NO_DEFAULT_PATH
    )
    find_path(FLUIDSYNTH_INCLUDE_DIR
        NAMES fluidsynth.h
        PATHS /opt/homebrew/include /usr/local/include
        NO_DEFAULT_PATH
    )

    if(FLUIDSYNTH_LIBRARY AND FLUIDSYNTH_INCLUDE_DIR)
        set(FLUIDSYNTH_FOUND TRUE)
        set(FLUIDSYNTH_LIBRARIES ${FLUIDSYNTH_LIBRARY})
        set(FLUIDSYNTH_INCLUDE_DIRS ${FLUIDSYNTH_INCLUDE_DIR})

        # Get version from header if possible
        if(EXISTS "${FLUIDSYNTH_INCLUDE_DIR}/fluidsynth/version.h")
            file(STRINGS "${FLUIDSYNTH_INCLUDE_DIR}/fluidsynth/version.h"
                 FLUIDSYNTH_VERSION_LINE
                 REGEX "^#define[ \t]+FLUIDSYNTH_VERSION[ \t]+\"[^\"]*\"")
            string(REGEX REPLACE "^#define[ \t]+FLUIDSYNTH_VERSION[ \t]+\"([^\"]*)\".*" "\\1"
                   FLUIDSYNTH_VERSION "${FLUIDSYNTH_VERSION_LINE}")
        endif()

        message(STATUS "Found FluidSynth via Homebrew: ${FLUIDSYNTH_LIBRARY}")
    endif()
endif()

# Use bundled RtMidi (no external dependency needed!)
message(STATUS "Using bundled RtMidi from external/rtmidi")
add_subdirectory(external/rtmidi)

# Use bundled Dear ImGui for GUI
message(STATUS "Using bundled Dear ImGui from external/imgui")
add_subdirectory(external/imgui)

# Add subdirectories
add_subdirectory(src/core)
add_subdirectory(src/hardware)
add_subdirectory(src/lua_bridge)
add_subdirectory(src/desktop)

# Enable testing
enable_testing()
add_subdirectory(tests)

# Print configuration
message(STATUS "=== GRUVBOK Configuration ===")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Lua: ${LUA_LIBRARIES}")
message(STATUS "SDL2: ${SDL2_LIBRARIES}")
message(STATUS "RtMidi: bundled")
message(STATUS "Dear ImGui: bundled")
if(FLUIDSYNTH_FOUND)
    message(STATUS "FluidSynth: ${FLUIDSYNTH_VERSION} (internal audio enabled)")
else()
    message(STATUS "FluidSynth: not found (MIDI-only mode)")
endif()
message(STATUS "=============================")
