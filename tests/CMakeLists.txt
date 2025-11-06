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
# add_executable(test_pattern test_pattern.cpp)
# target_link_libraries(test_pattern PRIVATE gruvbok_core)
# add_test(NAME PatternTests COMMAND test_pattern)
