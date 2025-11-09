# Core library
add_library(gruvbok_core STATIC
    event.cpp
    pattern.cpp
    song.cpp
    engine.cpp
    led_controller.cpp
)

target_include_directories(gruvbok_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${LUA_INCLUDE_DIR}
)
