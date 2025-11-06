# Desktop executable
add_executable(gruvbok
    main.cpp
    desktop_hardware.cpp
)

target_include_directories(gruvbok PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

if(RTMIDI_FOUND)
    target_include_directories(gruvbok PRIVATE ${RTMIDI_INCLUDE_DIR})
    target_link_libraries(gruvbok PRIVATE ${RTMIDI_LIBRARY})
else()
    message(FATAL_ERROR "RtMidi is required. Install with: brew install rtmidi")
endif()

target_link_libraries(gruvbok PRIVATE
    gruvbok_core
    gruvbok_hardware
    gruvbok_lua
)

# macOS specific frameworks
if(APPLE)
    target_link_libraries(gruvbok PRIVATE
        "-framework CoreMIDI"
        "-framework CoreAudio"
        "-framework CoreFoundation"
    )
endif()

# Set output directory
set_target_properties(gruvbok PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Installation
install(TARGETS gruvbok DESTINATION bin)
