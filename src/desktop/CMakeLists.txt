# Desktop executable
add_executable(gruvbok
    main.cpp
    desktop_hardware.cpp
)

target_include_directories(gruvbok PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

if(RTMIDI_FOUND)
    target_include_directories(gruvbok PRIVATE ${RTMIDI_INCLUDE_DIRS})
    target_link_libraries(gruvbok PRIVATE ${RTMIDI_LIBRARIES})
else()
    message(FATAL_ERROR "\n"
        "RtMidi is required but was not found.\n"
        "\n"
        "Install with:\n"
        "  brew install rtmidi\n"
        "\n"
        "If already installed, check installation:\n"
        "  brew info rtmidi\n"
        "  ls -l \$(brew --prefix)/lib/librtmidi*\n"
        "  ls -l \$(brew --prefix)/include/RtMidi.h\n"
        "\n"
        "If the files exist but CMake still can't find them, try:\n"
        "  cmake .. -DRTMIDI_INCLUDE_DIRS=\$(brew --prefix)/include -DRTMIDI_LIBRARIES=\$(brew --prefix)/lib/librtmidi.dylib\n"
    )
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
