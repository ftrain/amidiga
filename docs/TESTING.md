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

All tests pass: **73/73** ✅

### Test Suites

**test_event** (9 tests) - Event bit-packing
- Switch and pot get/set operations
- Value clamping (0-127)
- Bit isolation and independence
- Memory size validation (32 bits)

**test_pattern** (12 tests) - Pattern/Track hierarchy
- Track and Pattern containers
- Event isolation
- Clear functionality
- Full data patterns

**test_song** (12 tests) - Song/Mode hierarchy
- Mode and Song structure
- Memory footprint (245,760 bytes)
- Full hierarchy navigation
- Save/load roundtrip with boundaries

**test_midi_scheduler** (15 tests) - MIDI timing
- Note On/Off, CC, All Notes Off
- Delta-timed event scheduling
- Priority queue ordering
- MIDI Clock, Start, Stop, Continue

**test_engine** (8 tests) - Playback engine
- Step progression and tempo
- LED tempo beat pattern
- MIDI clock generation (24 PPQN)
- Start/Stop behavior

**test_lua_integration** (17 tests) - Lua bridge ⭐ NEW
- Lua version detection (5.1 or 5.4)
- LUA_OK constant compatibility
- Lua state creation and API registration
- Script loading with error handling
- Init context parameter passing
- Event processing and MIDI generation
- Lua 5.1 compatibility (no //, &, |, ~ operators)

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

Potential additions:
- [x] `test_lua_integration.cpp` - End-to-end Lua mode loading and execution ✅ **ADDED**
- [ ] `test_hardware_abstraction.cpp` - Hardware interface implementations
- [ ] `test_mode_scripts.cpp` - Validate all 15 mode Lua scripts
- [ ] Performance/stress tests for real-time constraints
- [ ] Memory leak tests (valgrind integration)

## CI/CD Integration

To run tests automatically on each commit:
```bash
# In your CI/CD pipeline
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Exit code 0 = all tests passed, non-zero = failures.
