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

# Try to use pkg-config to find RtMidi
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(RTMIDI rtmidi)
endif()

# If pkg-config didn't find it, search manually
if(NOT RTMIDI_FOUND)
    # Detect Homebrew prefix
    if(APPLE)
        execute_process(
            COMMAND brew --prefix
            OUTPUT_VARIABLE HOMEBREW_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(HOMEBREW_PREFIX)
            list(APPEND RTMIDI_SEARCH_PATHS "${HOMEBREW_PREFIX}/include")
            list(APPEND RTMIDI_LIB_SEARCH_PATHS "${HOMEBREW_PREFIX}/lib")
        endif()
    endif()

    find_path(RTMIDI_INCLUDE_DIR RtMidi.h
        PATHS ${RTMIDI_SEARCH_PATHS}
              /usr/local/include
              /usr/include
              /opt/homebrew/include
              /opt/local/include
    )

    find_library(RTMIDI_LIBRARY
        NAMES rtmidi
        PATHS ${RTMIDI_LIB_SEARCH_PATHS}
              /usr/local/lib
              /usr/lib
              /opt/homebrew/lib
              /opt/local/lib
    )

    if(RTMIDI_INCLUDE_DIR AND RTMIDI_LIBRARY)
        set(RTMIDI_FOUND TRUE)
        set(RTMIDI_INCLUDE_DIRS ${RTMIDI_INCLUDE_DIR})
        set(RTMIDI_LIBRARIES ${RTMIDI_LIBRARY})
    endif()
endif()

if(RTMIDI_FOUND)
    message(STATUS "Found RtMidi: ${RTMIDI_LIBRARIES}")
    message(STATUS "  Include: ${RTMIDI_INCLUDE_DIRS}")
else()
    message(WARNING "RtMidi not found. Searched in:")
    message(WARNING "  Include: ${RTMIDI_SEARCH_PATHS} /usr/local/include /opt/homebrew/include")
    message(WARNING "  Library: ${RTMIDI_LIB_SEARCH_PATHS} /usr/local/lib /opt/homebrew/lib")
    message(WARNING "")
    message(WARNING "Install RtMidi with: brew install rtmidi")
    message(WARNING "")
    message(WARNING "If already installed, try:")
    message(WARNING "  brew info rtmidi")
    message(WARNING "  ls -l \$(brew --prefix)/lib/librtmidi*")
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
    message(STATUS "RtMidi include: ${RTMIDI_INCLUDE_DIRS}")
    message(STATUS "RtMidi library: ${RTMIDI_LIBRARIES}")
else()
    message(STATUS "RtMidi: NOT FOUND")
endif()
message(STATUS "=============================")
