# Test suite for GRUVBOK

# Test executable for Event class
add_executable(test_event test_event.cpp)
target_link_libraries(test_event PRIVATE gruvbok_core)
target_include_directories(test_event PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME EventTests COMMAND test_event)

# Set output directory for tests
set_target_properties(test_event
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

# Add more tests here as needed
add_executable(test_pattern test_pattern.cpp)
target_link_libraries(test_pattern PRIVATE gruvbok_core)
target_include_directories(test_pattern PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME PatternTests COMMAND test_pattern)
set_target_properties(test_pattern
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

add_executable(test_song test_song.cpp)
target_link_libraries(test_song PRIVATE gruvbok_core)
target_include_directories(test_song PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME SongTests COMMAND test_song)
set_target_properties(test_song
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

add_executable(test_midi_scheduler test_midi_scheduler.cpp)
target_link_libraries(test_midi_scheduler PRIVATE gruvbok_hardware)  # Includes gruvbok_core transitively
target_include_directories(test_midi_scheduler PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME MidiSchedulerTests COMMAND test_midi_scheduler)
set_target_properties(test_midi_scheduler
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

add_executable(test_engine test_engine.cpp)
target_link_libraries(test_engine PRIVATE gruvbok_lua ${LUA_LIBRARIES})  # Includes gruvbok_hardware and gruvbok_core transitively
target_include_directories(test_engine PRIVATE ${CMAKE_SOURCE_DIR}/src ${LUA_INCLUDE_DIR})
add_test(NAME EngineTests COMMAND test_engine)
set_target_properties(test_engine
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

add_executable(test_lua_integration test_lua_integration.cpp)
target_link_libraries(test_lua_integration PRIVATE gruvbok_lua ${LUA_LIBRARIES})  # Includes gruvbok_hardware and gruvbok_core transitively
target_include_directories(test_lua_integration PRIVATE ${CMAKE_SOURCE_DIR}/src ${LUA_INCLUDE_DIR})
add_test(NAME LuaIntegrationTests COMMAND test_lua_integration)
set_target_properties(test_lua_integration
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

# New component tests (after refactoring)
add_executable(test_midi_clock_manager test_midi_clock_manager.cpp)
target_link_libraries(test_midi_clock_manager PRIVATE gruvbok_hardware)  # Includes gruvbok_core transitively
target_include_directories(test_midi_clock_manager PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME MidiClockManagerTests COMMAND test_midi_clock_manager)
set_target_properties(test_midi_clock_manager
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

add_executable(test_mode0_sequencer test_mode0_sequencer.cpp)
target_link_libraries(test_mode0_sequencer PRIVATE gruvbok_core)
target_include_directories(test_mode0_sequencer PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME Mode0SequencerTests COMMAND test_mode0_sequencer)
set_target_properties(test_mode0_sequencer
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

add_executable(test_playback_state test_playback_state.cpp)
target_link_libraries(test_playback_state PRIVATE gruvbok_hardware)  # Includes gruvbok_core transitively
target_include_directories(test_playback_state PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME PlaybackStateTests COMMAND test_playback_state)
set_target_properties(test_playback_state
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

# Memory profiler (not a test, but a diagnostic tool)
add_executable(memory_profiler memory_profiler.cpp)
target_link_libraries(memory_profiler PRIVATE gruvbok_lua ${LUA_LIBRARIES})  # Full stack
target_include_directories(memory_profiler PRIVATE ${CMAKE_SOURCE_DIR}/src ${LUA_INCLUDE_DIR})
set_target_properties(memory_profiler
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests"
)

