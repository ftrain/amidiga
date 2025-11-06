# GRUVBOK Test Suite

## Overview

This directory contains unit and integration tests for GRUVBOK core components.

## Running Tests

### Build and run all tests:
```bash
cd build
cmake ..
make
ctest
```

### Run tests with verbose output:
```bash
ctest --verbose
```

### Run a specific test:
```bash
./bin/tests/test_event
```

## Current Tests

### `test_event` - Event Bit-Packing Tests
Tests the core Event class (29-bit packed structure):
- ✅ Default constructor
- ✅ Switch get/set operations
- ✅ Pot (slider) get/set operations
- ✅ Value clamping (0-127 range)
- ✅ Bit isolation (setting one field doesn't affect others)
- ✅ Switch and pots independence
- ✅ Maximum values
- ✅ Memory size (must fit in 32 bits)
- ✅ Copy operations

All tests pass: **9/9** ✅

## Adding New Tests

1. Create a new test file in `tests/`, e.g., `test_pattern.cpp`
2. Use the simple test framework:
   ```cpp
   #include "../src/core/pattern.h"
   #include <iostream>
   #include <cassert>

   #define TEST(name) ...
   #define ASSERT_EQ(a, b) ...
   #define ASSERT_TRUE(expr) ...

   TEST(my_test_name) {
       // Test code here
       ASSERT_EQ(1 + 1, 2);
   }

   int main() {
       run_test_my_test_name();
       return (fail_count == 0) ? 0 : 1;
   }
   ```

3. Add to `tests/CMakeLists.txt`:
   ```cmake
   add_executable(test_pattern test_pattern.cpp)
   target_link_libraries(test_pattern PRIVATE gruvbok_core)
   add_test(NAME PatternTests COMMAND test_pattern)
   ```

4. Run `cmake --build build && ctest`

## Test Philosophy

- **Fast**: Tests should run in milliseconds
- **Isolated**: Each test is independent
- **Clear**: Test names describe what they verify
- **Comprehensive**: Cover edge cases and error conditions

## Future Tests

Planned test coverage:
- [ ] `test_pattern.cpp` - Pattern/Track/Event container tests
- [ ] `test_song.cpp` - Song data structure tests
- [ ] `test_midi_scheduler.cpp` - MIDI timing and scheduling
- [ ] `test_engine.cpp` - Engine playback logic (requires mocking)
- [ ] `test_lua_integration.cpp` - Lua API and mode loading

## CI/CD Integration

To run tests automatically on each commit:
```bash
# In your CI/CD pipeline
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Exit code 0 = all tests passed, non-zero = failures.
