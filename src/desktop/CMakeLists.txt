# Desktop executable
add_executable(gruvbok
    main.cpp
    desktop_hardware.cpp
)

target_include_directories(gruvbok PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(gruvbok PRIVATE
    gruvbok_core
    gruvbok_hardware
    gruvbok_lua
    rtmidi
    ${LUA_LIBRARIES}
)

# Set output directory
set_target_properties(gruvbok PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Installation
install(TARGETS gruvbok DESTINATION bin)
