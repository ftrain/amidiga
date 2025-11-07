# Hardware library
add_library(gruvbok_hardware STATIC
    midi_scheduler.cpp
    audio_output.cpp
)

target_include_directories(gruvbok_hardware PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(gruvbok_hardware PUBLIC
    gruvbok_core
)

# Optional FluidSynth support
if(FLUIDSYNTH_FOUND)
    target_compile_definitions(gruvbok_hardware PUBLIC HAVE_FLUIDSYNTH)
    target_include_directories(gruvbok_hardware PUBLIC ${FLUIDSYNTH_INCLUDE_DIRS})
    if(FLUIDSYNTH_LIBRARY_DIRS)
        target_link_directories(gruvbok_hardware PUBLIC ${FLUIDSYNTH_LIBRARY_DIRS})
    endif()
    target_link_libraries(gruvbok_hardware PUBLIC ${FLUIDSYNTH_LIBRARIES})
endif()
