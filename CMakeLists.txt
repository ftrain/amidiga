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
message(STATUS "=============================")
