# Hardware library
add_library(gruvbok_hardware STATIC
    midi_scheduler.cpp
)

target_include_directories(gruvbok_hardware PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(gruvbok_hardware PUBLIC
    gruvbok_core
)
