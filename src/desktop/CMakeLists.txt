# Console executable (original)
add_executable(gruvbok-console
    main.cpp
    desktop_hardware.cpp
)

target_include_directories(gruvbok-console PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(gruvbok-console PRIVATE
    gruvbok_core
    gruvbok_hardware
    gruvbok_lua
    rtmidi
    ${LUA_LIBRARIES}
)

# GUI executable (new!)
find_package(SDL2 REQUIRED)

add_executable(gruvbok
    gui_main.cpp
    desktop_hardware.cpp
)

target_include_directories(gruvbok PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${LUA_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(gruvbok PRIVATE
    gruvbok_core
    gruvbok_hardware
    gruvbok_lua
    rtmidi
    imgui
    ${LUA_LIBRARIES}
    ${SDL2_LIBRARIES}
)

# Set output directory
set_target_properties(gruvbok gruvbok-console PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Installation
install(TARGETS gruvbok gruvbok-console DESTINATION bin)
