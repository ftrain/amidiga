# GRUVBOK: Immediate Action Items

**Date**: 2025-11-09
**Priority**: High
**Estimated Time**: 10-15 hours total

---

## Quick Summary

A comprehensive code review revealed GRUVBOK has excellent architecture but opportunities for improvement. This document provides actionable steps to address the highest-priority issues.

---

## 1. Complete HardwareBase Migration (3-4 hours) 🔥

**Why**: Currently 220+ lines of code are duplicated across 4 platform implementations.

**What**: Refactor remaining 3 platforms to inherit from HardwareBase (already done for Desktop).

### Steps:

#### A. Refactor MacOSHardware (1 hour)

**File**: `native/GRUVBOK/macOS/MacOSHardware.h` and `MacOSHardware.mm`

```objc
// MacOSHardware.h - Change inheritance
#include "../../../../src/hardware/hardware_base.h"

class MacOSHardware : public HardwareBase {  // Changed from HardwareInterface
    // Remove: buttons_, rotary_pots_, slider_pots_, led_state_, start_time_
    // Remove duplicate method declarations (readButton, setLED, etc.)
    // Keep only: MIDI-specific members
};
```

```objc
// MacOSHardware.mm
MacOSHardware::MacOSHardware() {
    // Remove button/pot initialization (handled by HardwareBase)
}

// Remove these methods (inherited from HardwareBase):
// - readButton()
// - readRotaryPot()
// - readSliderPot()
// - setLED()
// - getLED()
// - getMillis()
// - simulateButton()
// - simulateRotaryPot()
// - simulateSliderPot()
```

#### B. Refactor IOSHardware (1 hour)

Same as MacOSHardware - identical pattern.

#### C. Refactor TeensyHardware (1-2 hours)

**File**: `src/teensy/teensy_hardware.h` and `teensy_hardware.cpp`

```cpp
// teensy_hardware.h
#include "../hardware/hardware_base.h"

class TeensyHardware : public HardwareBase {
public:
    // Override only these methods:
    bool init() override;
    void shutdown() override;
    void sendMidiMessage(const MidiMessage& msg) override;
    void update() override;  // Read actual hardware GPIO/ADC
    uint32_t getMillis() override;  // Use Arduino millis()

private:
    // Keep hardware-specific members:
    static constexpr int BUTTON_PINS[16] = {...};
    static constexpr int ROTARY_POT_PINS[4] = {...};
    // Remove: button_states_, pot_values_, led_state_ (in base)
};
```

**Key Changes in update()**:
```cpp
void TeensyHardware::update() {
    // Read hardware buttons and update base class state
    for (int i = 0; i < 16; i++) {
        buttons_[i] = !digitalRead(BUTTON_PINS[i]);  // Active low with pullup
    }

    // Read hardware pots and update base class state
    for (int i = 0; i < 4; i++) {
        uint16_t adc = analogRead(ROTARY_POT_PINS[i]);
        rotary_pots_[i] = mapAdcToMidi(adc, 1023);  // Use helper from base
    }

    // Set physical LED to match base class state
    digitalWrite(LED_PIN, led_state_ ? HIGH : LOW);
}
```

**Verification**:
```bash
# Test on each platform
cd build && ctest --verbose  # Should still pass 56 tests
```

---

## 2. Add Lua Integration Tests (2-3 hours) 🔥

**Why**: Lua integration is completely untested - critical gap!

**What**: Implement tests in `tests/test_lua_integration.cpp`

### Create Test File:

```cpp
// tests/test_lua_integration.cpp
#include "../src/lua_bridge/lua_context.h"
#include "../src/lua_bridge/mode_loader.h"
#include <iostream>
#include <fstream>

// Test framework (same as other tests)
#define TEST(name) void test_##name()
#define ASSERT_TRUE(expr) if (!(expr)) { throw std::runtime_error(...); }

TEST(lua_context_creation) {
    LuaContext ctx;
    ASSERT_TRUE(ctx.getState() != nullptr);
}

TEST(lua_load_valid_script) {
    // Create temporary Lua file
    std::ofstream out("/tmp/test_mode.lua");
    out << "function init(ctx) end\n";
    out << "function process_event(track, event) return {} end\n";
    out.close();

    LuaContext ctx;
    ASSERT_TRUE(ctx.loadScript("/tmp/test_mode.lua"));
    ASSERT_TRUE(ctx.isValid());
}

TEST(lua_missing_init_function) {
    std::ofstream out("/tmp/bad_mode.lua");
    out << "function process_event(track, event) return {} end\n";  // Missing init
    out.close();

    LuaContext ctx;
    ASSERT_FALSE(ctx.loadScript("/tmp/bad_mode.lua"));
    ASSERT_FALSE(ctx.isValid());
}

TEST(lua_process_event_basic) {
    std::ofstream out("/tmp/drum_mode.lua");
    out << R"(
        function init(ctx) end
        function process_event(track, event)
            if event.switch then
                note(60, 100, 0)
                off(60, 10)
            end
            return {}
        end
    )";
    out.close();

    LuaContext ctx;
    ctx.setChannel(0);
    ASSERT_TRUE(ctx.loadScript("/tmp/drum_mode.lua"));

    Event evt(true, 64, 64, 64, 64);  // Switch on
    auto midi_events = ctx.callProcessEvent(0, evt);

    ASSERT_TRUE(midi_events.size() >= 2);  // Note on + off
}

TEST(mode_loader_directory) {
    ModeLoader loader;
    int loaded = loader.loadModesFromDirectory("modes/", 120);
    ASSERT_TRUE(loaded > 0);  // Should load some modes
    ASSERT_TRUE(loader.isModeLoaded(1));  // Mode 1 (drums)
}

// Add to main():
int main() {
    run_test_lua_context_creation();
    run_test_lua_load_valid_script();
    run_test_lua_missing_init_function();
    run_test_lua_process_event_basic();
    run_test_mode_loader_directory();

    std::cout << "Lua tests: " << pass_count << "/" << test_count << " passed\n";
    return fail_count > 0 ? 1 : 0;
}
```

### Update CMakeLists.txt:

```cmake
# tests/CMakeLists.txt
add_executable(test_lua_integration
    test_lua_integration.cpp
)
target_link_libraries(test_lua_integration PRIVATE
    gruvbok_lua
)
add_test(NAME lua_integration COMMAND test_lua_integration)
```

**Expected Outcome**: +5-10 tests, covering critical Lua functionality.

---

## 3. Centralize Configuration (1-2 hours)

**Why**: Magic numbers scattered throughout code make tuning difficult.

**What**: Create central config file.

### Create Config Header:

```cpp
// src/core/config.h
#pragma once

#include <cstdint>

namespace gruvbok {
namespace config {

// ============================================================================
// Timing Constants
// ============================================================================

/// Autosave interval (milliseconds)
constexpr uint32_t AUTOSAVE_INTERVAL_MS = 20000;

/// LED tempo indicator duration (milliseconds)
constexpr uint32_t LED_TEMPO_DURATION_MS = 50;

/// Tempo change debounce delay (milliseconds)
constexpr uint32_t TEMPO_DEBOUNCE_MS = 1000;

// ============================================================================
// Musical Constants
// ============================================================================

/// Minimum tempo (BPM)
constexpr int TEMPO_MIN_BPM = 1;

/// Maximum tempo (BPM)
constexpr int TEMPO_MAX_BPM = 1000;

/// Default tempo (BPM)
constexpr int TEMPO_DEFAULT_BPM = 120;

/// MIDI clock pulses per quarter note (MIDI standard)
constexpr int MIDI_PPQN = 24;

// ============================================================================
// Hardware Constants
// ============================================================================

/// Number of button inputs (B1-B16)
constexpr int NUM_BUTTONS = 16;

/// Number of rotary pots (R1-R4)
constexpr int NUM_ROTARY_POTS = 4;

/// Number of slider pots (S1-S4)
constexpr int NUM_SLIDER_POTS = 4;

/// MIDI value range maximum
constexpr int MIDI_MAX_VALUE = 127;

// ============================================================================
// Data Structure Sizes
// ============================================================================

/// Events per track (matches button count)
constexpr int EVENTS_PER_TRACK = 16;

/// Tracks per pattern
constexpr int TRACKS_PER_PATTERN = 8;

/// Patterns per mode
constexpr int PATTERNS_PER_MODE = 32;

/// Total modes in song
constexpr int NUM_MODES = 15;

} // namespace config
} // namespace gruvbok
```

### Update Engine to Use Config:

```cpp
// src/core/engine.cpp
#include "config.h"

// Replace:
static constexpr uint32_t AUTOSAVE_INTERVAL_MS = 20000;
// With:
using namespace gruvbok::config;

// Then use: AUTOSAVE_INTERVAL_MS, TEMPO_DEBOUNCE_MS, etc.
```

**Estimated Changes**: ~20-30 call sites across 5-6 files.

---

## 4. Add Inline Documentation (2-3 hours)

**Why**: Many public APIs lack usage documentation.

**What**: Add Doxygen comments to critical classes.

### Priority Classes to Document:

1. **Engine** (`src/core/engine.h`)
```cpp
/**
 * @brief Main playback engine for GRUVBOK sequencer
 *
 * The Engine orchestrates real-time playback by:
 * - Stepping through events at tempo-synchronized intervals
 * - Calling Lua modes to process events and generate MIDI
 * - Scheduling MIDI output via MidiScheduler
 * - Generating MIDI clock messages (24 PPQN)
 * - Managing LED tempo indicators
 * - Handling autosave for dirty data
 *
 * Thread-safety: Not thread-safe. All methods must be called from the same thread.
 *
 * Typical usage:
 * @code
 * Song song;
 * DesktopHardware hardware;
 * ModeLoader modes;
 * Engine engine(&song, &hardware, &modes);
 *
 * engine.start();
 * while (running) {
 *     engine.update();  // Call at ~60fps
 *     // ... GUI rendering ...
 * }
 * engine.stop();
 * @endcode
 */
class Engine {
    /**
     * @brief Update engine state and process scheduled events
     *
     * Call this method frequently (e.g., in main loop at ~60fps).
     * Handles:
     * - MIDI clock generation
     * - Step advancement at tempo intervals
     * - Lua mode processing
     * - LED pattern updates
     * - Autosave checks
     *
     * @note Must be called regularly for accurate timing
     */
    void update();
```

2. **MidiScheduler** (`src/hardware/midi_scheduler.h`)
```cpp
/**
 * @brief Schedules and sends delta-timed MIDI events
 *
 * Converts relative (delta) timing to absolute timestamps and sends
 * MIDI messages at precise times. Supports routing to:
 * - External MIDI ports (via HardwareInterface)
 * - Internal audio synthesis (via AudioOutput/FluidSynth)
 *
 * Events are stored in a priority queue sorted by absolute time.
 * Call update() frequently to send due events.
 */
```

3. **LuaContext** (`src/lua_bridge/lua_context.h`)
```cpp
/**
 * @brief Wrapper around lua_State for a single musical mode
 *
 * Manages lifecycle of Lua state including:
 * - Loading and validating Lua scripts
 * - Calling init(context) function
 * - Calling process_event(track, event) function
 * - Accumulating MIDI events in internal buffer
 *
 * Lifecycle:
 * 1. Create LuaContext
 * 2. loadScript(filepath)
 * 3. callInit(context)
 * 4. callProcessEvent(...) - repeatedly during playback
 *
 * Thread-safety: Not thread-safe. Lua state is not thread-safe.
 *
 * @see LuaAPI for functions available to Lua scripts
 */
```

---

## 5. Performance Instrumentation (1 hour)

**Why**: Need visibility into timing for optimization.

**What**: Add scoped timing utility.

### Create Timing Utility:

```cpp
// src/core/timing.h
#pragma once

#include <chrono>
#include <iostream>
#include <string>

namespace gruvbok {

/**
 * @brief RAII timer for performance measurement
 *
 * Logs a warning if operation exceeds threshold.
 *
 * Usage:
 * @code
 * void expensiveFunction() {
 *     ScopedTimer timer("expensiveFunction", 1000);  // Warn if > 1ms
 *     // ... do work ...
 * } // Destructor logs if exceeded threshold
 * @endcode
 */
class ScopedTimer {
public:
    ScopedTimer(const char* name, uint32_t threshold_us = 1000)
        : name_(name)
        , threshold_us_(threshold_us)
        , start_(std::chrono::steady_clock::now()) {}

    ~ScopedTimer() {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
        uint32_t us = static_cast<uint32_t>(duration.count());

        if (us > threshold_us_) {
            std::cerr << "[TIMING] " << name_ << " took " << us << "μs (threshold: " << threshold_us_ << "μs)\n";
        }
    }

private:
    const char* name_;
    uint32_t threshold_us_;
    std::chrono::steady_clock::time_point start_;
};

} // namespace gruvbok
```

### Instrument Critical Paths:

```cpp
// src/core/engine.cpp
#include "timing.h"

void Engine::update() {
    ScopedTimer timer("Engine::update", 1000);  // Warn if > 1ms
    // ... existing code ...
}

void Engine::processStep() {
    ScopedTimer timer("Engine::processStep", 500);  // Warn if > 500μs
    // ... existing code ...
}
```

**Expected Output** (only if exceeds threshold):
```
[TIMING] Engine::processStep took 1234μs (threshold: 500μs)
```

---

## Testing Checklist

After implementing each action item:

- [ ] Code compiles without errors
- [ ] All existing tests still pass (56/56)
- [ ] New tests added and passing
- [ ] Desktop GUI works
- [ ] macOS app works (if migrated)
- [ ] iOS app works (if migrated)
- [ ] No performance regression (timing < 1ms per update)

---

## Commit Strategy

### Branch
Already on: `claude/code-review-thorough-011CUwaS19dUPVate9qKzirx`

### Commit Messages

```bash
# First commit (already done)
git add docs/CODE_REVIEW.md docs/REFACTORING_SUMMARY.md docs/IMMEDIATE_ACTIONS.md
git add src/hardware/hardware_base.{h,cpp}
git add src/hardware/CMakeLists.txt
git add src/desktop/desktop_hardware.{h,cpp}
git commit -m "Add comprehensive code review and HardwareBase refactoring

- Add detailed CODE_REVIEW.md with architecture analysis
- Add REFACTORING_SUMMARY.md documenting changes
- Add IMMEDIATE_ACTIONS.md with prioritized tasks
- Create HardwareBase class to eliminate platform duplication
- Refactor DesktopHardware to inherit from HardwareBase
- Remove 80+ lines of duplicate code from DesktopHardware
- Add comprehensive Doxygen documentation to HardwareBase

Impact:
- Eliminates 220+ lines of duplication (when all platforms migrated)
- Improves code maintainability and consistency
- Provides clear foundation for platform implementations"

# Future commits
git commit -m "Migrate macOS/iOS/Teensy hardware to HardwareBase"
git commit -m "Add Lua integration tests (10 new tests)"
git commit -m "Centralize configuration constants"
git commit -m "Add comprehensive inline documentation"
git commit -m "Add performance instrumentation"
```

### Push to Remote

```bash
git push -u origin claude/code-review-thorough-011CUwaS19dUPVate9qKzirx
```

---

## Time Estimate

| Task | Priority | Est. Time |
|------|----------|-----------|
| 1. Complete HardwareBase migration | 🔥 High | 3-4 hours |
| 2. Add Lua integration tests | 🔥 High | 2-3 hours |
| 3. Centralize configuration | Medium | 1-2 hours |
| 4. Add inline documentation | Medium | 2-3 hours |
| 5. Performance instrumentation | Low | 1 hour |
| **TOTAL** | | **10-15 hours** |

---

## Success Criteria

✅ **All platforms use HardwareBase** (no duplicate button/pot code)
✅ **Lua integration fully tested** (10+ new tests passing)
✅ **No magic numbers in code** (all in config.h)
✅ **All public APIs documented** (Doxygen comments)
✅ **Performance monitored** (ScopedTimer in critical paths)
✅ **All tests passing** (66+ tests, 100% pass rate)

---

## Questions?

- See `docs/CODE_REVIEW.md` for detailed analysis
- See `docs/REFACTORING_SUMMARY.md` for what's been done
- See `CLAUDE.md` for overall architecture guide

**Ready to proceed!** 🚀
