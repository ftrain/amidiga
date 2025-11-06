# RtMidi library (bundled)

add_library(rtmidi STATIC
    RtMidi.cpp
)

# Platform-specific definitions
if(APPLE)
    target_compile_definitions(rtmidi PUBLIC __MACOSX_CORE__)
    target_link_libraries(rtmidi PUBLIC
        "-framework CoreMIDI"
        "-framework CoreAudio"
        "-framework CoreFoundation"
    )
elseif(UNIX)
    target_compile_definitions(rtmidi PUBLIC __LINUX_ALSA__)
    find_package(ALSA REQUIRED)
    target_link_libraries(rtmidi PUBLIC ${ALSA_LIBRARIES})
elseif(WIN32)
    target_compile_definitions(rtmidi PUBLIC __WINDOWS_MM__)
    target_link_libraries(rtmidi PUBLIC winmm)
endif()

# Export include directory
target_include_directories(rtmidi PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
