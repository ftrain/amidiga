# GRUVBOK Comprehensive Code Review

**Date**: 2025-11-09
**Reviewer**: Claude (Automated Deep Analysis)
**Codebase Version**: Phase 5 (Desktop Complete, Teensy Ready)

## Executive Summary

GRUVBOK is a well-architected hardware groovebox with a clean separation between core sequencing logic, hardware abstraction, and Lua-based musical modes. The codebase successfully targets three platforms: ImGui desktop, Swift (macOS/iOS), and Teensy 4.1 embedded hardware.

**Strengths:**
- ✅ Clean layered architecture (Core → Hardware → Lua Bridge)
- ✅ Excellent bit-packing optimization for memory efficiency
- ✅ Comprehensive test coverage (56 tests passing)
- ✅ Platform abstraction allows code reuse across desktop/mobile/embedded
- ✅ Lua integration provides flexibility for musical modes

**Areas for Improvement:**
- ⚠️ Engine class has too many responsibilities (violates SRP)
- ⚠️ Code duplication across platform implementations
- ⚠️ Inconsistent error handling patterns
- ⚠️ Magic numbers and hardcoded constants
- ⚠️ Limited inline documentation in some areas

---

## 1. Architecture Analysis

### 1.1 Core Design Patterns

The codebase follows a layered architecture:

```
┌─────────────────────────────────────┐
│   Platform Layer (GUI/Swift/HW)    │  ← Platform-specific UI
├─────────────────────────────────────┤
│         Engine (Orchestration)      │  ← Main playback loop
├─────────────────────────────────────┤
│    Lua Bridge (Musical Modes)       │  ← Scriptable behavior
├─────────────────────────────────────┤
│   Hardware Abstraction (MIDI/IO)    │  ← Abstract interface
├─────────────────────────────────────┤
│      Core (Event/Pattern/Song)      │  ← Pure data structures
└─────────────────────────────────────┘
```

**Strengths:**
- Clear separation of concerns
- Platform-agnostic core code
- Dependency injection (Engine receives pointers, not singletons)

**Issues:**
1. **Engine class violates SRP**: Handles playback, input, LED patterns, autosave, MIDI clock, Mode 0 parsing. Should be decomposed.
2. **Circular knowledge**: Engine knows about HardwareInterface, ModeLoader, MidiScheduler. These also know about Engine (via callbacks/pointers).
3. **God object pattern**: Engine is the central orchestrator for everything.

**Recommendation:**
Decompose Engine into:
- `PlaybackEngine` - Pure sequencing logic
- `InputHandler` - Button/pot input processing
- `LEDController` - LED pattern management
- `ClockGenerator` - MIDI clock generation
- `AutoSaver` - Persistence management

### 1.2 Dependency Graph

```
Engine ──┬──> Song (data)
         ├──> HardwareInterface (I/O)
         ├──> ModeLoader (Lua)
         └──> MidiScheduler (output)
                └──> AudioOutput (optional)

LuaContext ──> Engine (for LED control)
```

**Issues:**
- Bi-directional dependency: `Engine → LuaContext` and `LuaContext → Engine`
- Circular dependency creates tight coupling

**Recommendation:**
Use an event bus or observer pattern:
```cpp
class EngineEvents {
    virtual void onLEDTrigger(LEDPattern pattern, uint8_t brightness) = 0;
};

// Engine provides events, Lua consumes them
```

---

## 2. Code Quality Issues

### 2.1 Magic Numbers

**Location**: Multiple files

**Examples:**
```cpp
// src/core/engine.cpp:105
static constexpr uint32_t AUTOSAVE_INTERVAL_MS = 20000;  // 20 seconds

// src/core/engine.cpp:123
static constexpr uint32_t LED_TEMPO_DURATION_MS = 50;  // LED stays on for 50ms

// src/core/engine.cpp:128
static constexpr uint32_t TEMPO_DEBOUNCE_MS = 1000;  // Wait 1 second
```

**Issue**: Magic numbers are scattered throughout code without centralized configuration.

**Recommendation:**
Create a `src/core/config.h`:
```cpp
namespace gruvbok::config {
    constexpr uint32_t AUTOSAVE_INTERVAL_MS = 20000;
    constexpr uint32_t LED_TEMPO_DURATION_MS = 50;
    constexpr uint32_t TEMPO_DEBOUNCE_MS = 1000;
    constexpr int TEMPO_MIN_BPM = 1;
    constexpr int TEMPO_MAX_BPM = 1000;
    constexpr int MIDI_PPQN = 24;  // Standard 24 PPQN
}
```

### 2.2 Error Handling Inconsistencies

**Issue**: Mix of error handling patterns:
- Some functions return `bool` for success/failure
- Some throw exceptions (in JSON parsing)
- Some silently fail or log errors
- Some use assertions

**Examples:**
```cpp
// Pattern 1: Return bool
bool init() override;

// Pattern 2: Silent failure
void setMode(int mode) {
    if (mode >= 0 && mode < Song::NUM_MODES) {
        current_mode_ = mode;
    }  // Silently ignores invalid input
}

// Pattern 3: Assertion (debug only)
assert(pot >= 0 && pot < 4);
```

**Recommendation:**
Standardize on a pattern:
1. Use `std::optional<T>` or `std::expected<T, Error>` for failable operations
2. Reserve exceptions for truly exceptional cases
3. Document error behavior in function comments
4. Add validation helpers:

```cpp
namespace gruvbok::validation {
    inline bool isValidMode(int mode) {
        return mode >= 0 && mode < Song::NUM_MODES;
    }

    inline bool isValidTempo(int bpm) {
        return bpm >= config::TEMPO_MIN_BPM && bpm <= config::TEMPO_MAX_BPM;
    }
}
```

### 2.3 Naming Inconsistencies

**Issue**: Mix of naming conventions:
- C++ classes: `PascalCase` ✅
- Methods: `camelCase` ✅
- Variables: `snake_case` (with trailing underscore for members) ✅
- **BUT**: Some Lua-facing functions use `snake_case` (e.g., `process_event`)

**Recommendation**: Document naming convention in `docs/STYLE_GUIDE.md`:
- C++ API: `camelCase` for methods
- Lua API: `snake_case` for Lua-facing functions
- Members: `snake_case_` with trailing underscore
- Constants: `UPPER_SNAKE_CASE` or `kPascalCase`

---

## 3. Code Duplication

### 3.1 Platform Hardware Implementations

**Files with duplication:**
- `src/desktop/desktop_hardware.cpp`
- `native/GRUVBOK/macOS/MacOSHardware.mm`
- `native/GRUVBOK/iOS/IOSHardware.mm`
- `src/teensy/teensy_hardware.cpp`

**Duplicated logic:**
1. Button state storage (`std::array<bool, 16> buttons_`)
2. Pot value storage (`std::array<uint8_t, 4> rotary_pots_`, `slider_pots_`)
3. LED state management (`bool led_state_`)
4. Timing (`getMillis()` implementation pattern)
5. Button simulation (`simulateButton`, `simulateRotaryPot`, `simulateSliderPot`)

**Recommendation:**
Create a common `HardwareBase` class:

```cpp
// src/hardware/hardware_base.h
namespace gruvbok {

class HardwareBase : public HardwareInterface {
protected:
    std::array<bool, 16> buttons_{};
    std::array<uint8_t, 4> rotary_pots_{};
    std::array<uint8_t, 4> slider_pots_{};
    bool led_state_ = false;

    // Common helper methods
    uint8_t mapAdcToMidi(uint16_t adc_value, uint16_t adc_max);
    bool debounceButton(int button, bool current_state);

public:
    // Default implementations
    bool readButton(int button) override {
        return button >= 0 && button < 16 ? buttons_[button] : false;
    }

    uint8_t readRotaryPot(int pot) override {
        return pot >= 0 && pot < 4 ? rotary_pots_[pot] : 0;
    }

    uint8_t readSliderPot(int pot) override {
        return pot >= 0 && pot < 4 ? slider_pots_[pot] : 0;
    }

    void setLED(bool on) override {
        led_state_ = on;
    }

    bool getLED() const override {
        return led_state_;
    }

    // Virtual methods for platform-specific behavior
    virtual void sendMidiMessageImpl(const MidiMessage& msg) = 0;
    virtual uint32_t getMillisImpl() = 0;
};

} // namespace gruvbok
```

**Estimated LOC reduction**: ~200-300 lines across 4 files

### 3.2 Slider Label Duplication

**Issue**: `gui_main.cpp` has hardcoded slider labels for each mode.

**Files:**
- `src/desktop/gui_main.cpp:84-113` - `GetSliderLabel()` function
- Lua modes have `SLIDER_LABELS` global variable

**Recommendation:**
Query Lua mode for labels at runtime:

```cpp
// In Engine or GUI
std::vector<std::string> getSliderLabels(int mode) {
    LuaContext* ctx = mode_loader_->getMode(mode);
    if (ctx && ctx->isValid()) {
        return ctx->getSliderLabels();  // Already exists!
    }
    return {"S1", "S2", "S3", "S4"};
}
```

**Estimated LOC reduction**: ~30 lines

---

## 4. Performance & Latency Analysis

### 4.1 Real-Time Constraints

**Critical path** (must execute in < 1ms):
1. `Engine::update()` - Main loop
2. `processStep()` - Triggers Lua for all modes/tracks
3. `LuaContext::callProcessEvent()` - Lua execution
4. `MidiScheduler::update()` - Send due MIDI events

**Analysis:**

```cpp
void Engine::processStep() {
    // For each of 14 modes (modes 1-14)
    for (int mode_num = 1; mode_num < 15; ++mode_num) {
        // For each of 8 tracks
        for (int track = 0; track < 8; ++track) {
            // Call Lua (potential allocation, GC)
            auto midi_events = lua_mode->callProcessEvent(track, event);
            scheduler_->schedule(midi_events);  // Copy events
        }
    }
}
```

**Potential issues:**
1. **Lua GC pauses**: Not observed yet, but could occur on embedded
2. **Memory allocations**: `std::vector` allocations in `callProcessEvent`
3. **Priority queue operations**: `O(log n)` insertions in scheduler

**Measurements needed:**
- Profile `update()` loop on desktop (use `std::chrono::high_resolution_clock`)
- Test on Teensy with real MIDI output load
- Monitor Lua memory usage over time

**Recommendations:**
1. Pre-allocate Lua event buffers (already done ✅)
2. Configure Lua GC for incremental mode:
```cpp
lua_gc(L_, LUA_GCINC, 200, 200, 10);  // Incremental GC
```
3. Add timing instrumentation:
```cpp
class ScopedTimer {
    const char* name_;
    std::chrono::steady_clock::time_point start_;
public:
    ScopedTimer(const char* name) : name_(name), start_(std::chrono::steady_clock::now()) {}
    ~ScopedTimer() {
        auto duration = std::chrono::steady_clock::now() - start_;
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        if (us > 100) {  // Warn if > 100μs
            std::cerr << name_ << " took " << us << "μs\n";
        }
    }
};

void Engine::processStep() {
    ScopedTimer timer("processStep");
    // ...
}
```

### 4.2 Memory Usage

**Current footprint** (calculated from CLAUDE.md):
- Event data: ~245 KB (61,440 events × 4 bytes)
- Lua contexts: ~50-100 KB (15 Lua states)
- Code + stack: ~200-300 KB
- **Total**: < 600 KB / 1 MB (Teensy 4.1)

**Potential optimizations:**
1. Sparse event storage (only store events with `switch=true`)
2. Shared Lua state with separate globals (reduce 15 states to 1)
3. Compile Lua modes to bytecode and embed in firmware

**Recommendation:** Current memory usage is excellent. No changes needed unless Teensy testing reveals issues.

---

## 5. Testing Gaps

### 5.1 Current Test Coverage

**Test files:**
- `test_event.cpp` - 9 tests ✅
- `test_pattern.cpp` - 12 tests ✅
- `test_song.cpp` - 12 tests ✅
- `test_midi_scheduler.cpp` - 15 tests ✅
- `test_engine.cpp` - 8 tests ✅
- `test_lua_integration.cpp` - 0 tests (file exists but empty)

**Total:** 56 tests passing

### 5.2 Missing Tests

**Critical gaps:**
1. ❌ Lua integration tests (file exists but empty)
2. ❌ Mode 0 (song sequencer) logic tests
3. ❌ MIDI clock timing accuracy tests
4. ❌ LED pattern tests
5. ❌ Autosave functionality tests
6. ❌ Error handling / edge case tests
7. ❌ Platform hardware implementation tests

**Recommendation:**
Add tests for:

```cpp
// tests/test_lua_integration.cpp
TEST(lua_mode_loading) {
    // Test that modes load correctly
}

TEST(lua_process_event_basic) {
    // Test Lua processing returns valid MIDI
}

TEST(lua_error_handling) {
    // Test Lua script errors don't crash engine
}

// tests/test_mode0.cpp
TEST(mode0_pattern_sequencing) {
    // Test Mode 0 controls other mode patterns
}

TEST(mode0_global_parameters) {
    // Test scale root, scale type, velocity offsets
}

// tests/test_timing.cpp
TEST(midi_clock_accuracy) {
    // Test MIDI clock doesn't drift over time
}

TEST(tempo_change_handling) {
    // Test tempo changes recalculate intervals correctly
}
```

**Estimated new tests needed:** 20-30 tests

---

## 6. Documentation Gaps

### 6.1 Existing Documentation

**Comprehensive:**
- ✅ `CLAUDE.md` - Excellent architectural guide
- ✅ `docs/LUA_API.md` - Complete Lua API reference
- ✅ `PROJECT_STRUCTURE.md` - Clear project layout
- ✅ `DEVELOPMENT_ROADMAP.md` - Development status

**Minimal:**
- ⚠️ Inline code comments (Doxygen style) - Spotty coverage
- ⚠️ API documentation - Missing for many classes
- ⚠️ Platform porting guide - Not documented
- ⚠️ Contribution guidelines - Not present

### 6.2 Inline Documentation Issues

**Under-documented classes:**
- `Engine` - Many methods lack comments
- `MidiScheduler` - Implementation details not explained
- `LuaContext` - Lifecycle and thread-safety not documented
- Platform hardware classes - Pin mappings not in code

**Example of good documentation:**
```cpp
// src/hardware/audio_output.h
/**
 * @brief Audio output using FluidSynth for internal synthesis
 *
 * Wraps FluidSynth to provide internal audio rendering.
 * Supports loading SoundFonts and processing MIDI messages.
 */
class AudioOutput {
```

**Example of missing documentation:**
```cpp
// src/core/engine.h - No class-level docstring
class Engine {
public:
    void update();  // What does this do? When should it be called?
    void processStep();  // Private, but complex - needs explanation
```

**Recommendation:**
Add Doxygen comments to all public APIs:

```cpp
/**
 * @brief Main playback engine for GRUVBOK sequencer
 *
 * Orchestrates playback by:
 * - Stepping through events at tempo-sync'd intervals
 * - Calling Lua modes to process events
 * - Scheduling MIDI output via MidiScheduler
 * - Managing MIDI clock generation (24 PPQN)
 * - Handling LED tempo indicators
 *
 * @note Call update() frequently (e.g., in main loop at ~60fps)
 * @note Thread-safety: Not thread-safe. All methods must be called from same thread.
 */
class Engine {
    /**
     * @brief Start playback
     *
     * Resets playback position to step 0 and sends MIDI Start message.
     */
    void start();
```

---

## 7. Platform Consistency Issues

### 7.1 Cross-Platform Code Reuse

**Current approach:**
- Desktop (ImGui): `src/desktop/gui_main.cpp` + `desktop_hardware.cpp`
- macOS/iOS (Swift): `native/GRUVBOK/Shared/Bridge/EngineWrapper.mm`
- Teensy: `src/teensy/main.cpp` + `teensy_hardware.cpp`

**Issues:**
1. **Three separate main loops**: Each platform reimplements UI/update loop
2. **Wrapper duplication**: Swift wrapper has getter methods that duplicate Engine API
3. **Inconsistent initialization**: Different order of operations on each platform

**Example - Initialization differs:**

```cpp
// Desktop (gui_main.cpp)
song = std::make_unique<Song>();
hardware = std::make_unique<DesktopHardware>();
hardware->init();
mode_loader = std::make_unique<ModeLoader>();
mode_loader->loadModesFromDirectory(...);
engine = std::make_unique<Engine>(song.get(), hardware.get(), mode_loader.get());
engine->initAudioOutput(...);
engine->start();

// Swift (EngineWrapper.mm)
song_ = std::make_unique<Song>();
hardware_ = std::make_unique<MacOSHardware>();
hardware_->init();
modeLoader_ = std::make_unique<ModeLoader>();
modeLoader_->loadModesFromDirectory(...);
engine_ = std::make_unique<Engine>(song_.get(), hardware_.get(), modeLoader_.get());
hardware_->initAudio(...);  // Different! Goes through hardware, not engine
engine_->start();
```

**Recommendation:**
Create a unified `Application` or `GroovboxContext` class:

```cpp
// src/core/application.h
namespace gruvbok {

struct ApplicationConfig {
    std::string modes_directory;
    std::string soundfont_path;
    int initial_tempo = 120;
    bool use_internal_audio = true;
    bool use_external_midi = true;
};

class Application {
public:
    explicit Application(std::unique_ptr<HardwareInterface> hardware);

    bool initialize(const ApplicationConfig& config);
    void start();
    void stop();
    void update();  // Call at ~60fps

    // Accessors
    Engine* getEngine() { return engine_.get(); }
    Song* getSong() { return song_.get(); }
    HardwareInterface* getHardware() { return hardware_.get(); }

private:
    std::unique_ptr<Song> song_;
    std::unique_ptr<HardwareInterface> hardware_;
    std::unique_ptr<ModeLoader> mode_loader_;
    std::unique_ptr<Engine> engine_;
};

} // namespace gruvbok
```

Then each platform just creates hardware and passes it to Application:

```cpp
// Desktop
auto hardware = std::make_unique<DesktopHardware>();
auto app = std::make_unique<Application>(std::move(hardware));
app->initialize({.modes_directory = "modes/", ...});
app->start();

// In main loop:
app->update();
```

**Estimated LOC reduction**: ~100-150 lines across platforms

---

## 8. Specific Recommendations

### 8.1 High Priority (Do First)

1. **Extract Engine responsibilities** into separate controllers
   - Complexity: High
   - Impact: High (improves testability, maintainability)
   - LOC: Refactor ~500 lines

2. **Create HardwareBase class** to eliminate platform duplication
   - Complexity: Medium
   - Impact: High (reduces duplication across 4 files)
   - LOC: Remove ~250 lines, add ~100 lines base class

3. **Add comprehensive Lua integration tests**
   - Complexity: Medium
   - Impact: High (currently untested!)
   - LOC: Add ~200-300 lines of tests

4. **Create unified Application class**
   - Complexity: Medium
   - Impact: Medium (simplifies platform code)
   - LOC: Add ~150 lines, simplify 3 platform mains

### 8.2 Medium Priority

5. **Centralize magic numbers** in `config.h`
   - Complexity: Low
   - Impact: Medium (improves maintainability)
   - LOC: Add ~50 lines config, update ~20 call sites

6. **Add timing instrumentation** for performance monitoring
   - Complexity: Low
   - Impact: Medium (enables profiling)
   - LOC: Add ~50 lines, instrument ~10 functions

7. **Standardize error handling**
   - Complexity: Medium
   - Impact: Medium (improves robustness)
   - LOC: Update ~50 functions

8. **Improve inline documentation**
   - Complexity: Low
   - Impact: Medium (improves onboarding)
   - LOC: Add ~500 lines of docstrings

### 8.3 Low Priority (Nice to Have)

9. **Query Lua for slider labels** instead of hardcoding
   - Complexity: Low
   - Impact: Low (already works, just reduces duplication)
   - LOC: Remove ~30 lines

10. **Create STYLE_GUIDE.md**
    - Complexity: Low
    - Impact: Low (documentation)
    - LOC: Add ~200 lines docs

---

## 9. Security & Safety Considerations

### 9.1 Memory Safety

**Current state:**
- ✅ No raw pointers (uses `std::unique_ptr`, `std::array`)
- ✅ No manual `new`/`delete`
- ✅ RAII patterns throughout
- ⚠️ Some raw pointers passed as non-owning references (Engine → Song, Hardware)

**Recommendation:** Document ownership clearly:
```cpp
// src/core/engine.h
class Engine {
    /**
     * Non-owning pointers. Caller retains ownership and must ensure
     * these outlive the Engine instance.
     */
    Song* song_;
    HardwareInterface* hardware_;
    ModeLoader* mode_loader_;
```

### 9.2 Thread Safety

**Current state:**
- ⚠️ No explicit thread safety (single-threaded assumption)
- ⚠️ Audio callback runs on separate thread (FluidSynth)
- ⚠️ Lua execution not protected by mutex

**Recommendation:**
Document thread safety requirements:
```cpp
/**
 * @note Thread-safety: Not thread-safe. All methods must be called
 * from the same thread. Audio output (FluidSynth) runs on a separate
 * thread but is internally synchronized.
 */
class Engine { ... };
```

---

## 10. Conclusion

**Overall Grade: A-**

GRUVBOK is a well-designed, functional codebase with excellent architecture and platform abstraction. The code successfully runs on three very different platforms (desktop, mobile, embedded) with minimal duplication.

**Key strengths:**
- Clean layered architecture
- Excellent bit-packing for memory efficiency
- Good test coverage for core data structures
- Platform abstraction enables true code reuse

**Key areas for improvement:**
- Decompose Engine class (SRP violation)
- Reduce platform implementation duplication
- Add Lua integration tests
- Improve inline documentation
- Centralize configuration

**Recommended next steps:**
1. Implement HardwareBase class (2-3 hours)
2. Add Lua integration tests (2-3 hours)
3. Extract LED/Clock/Autosave from Engine (4-6 hours)
4. Create Application class (2-3 hours)
5. Add comprehensive Doxygen comments (4-6 hours)

**Total estimated effort:** 15-20 hours for all high-priority improvements

---

## Appendix: Metrics

**Code Statistics:**
- Total lines of code: ~107,000 (including external libraries)
- GRUVBOK source: ~3,600 lines
- Test code: ~1,000 lines
- Lua modes: ~1,500 lines
- Test pass rate: 100% (56/56)

**File Counts:**
- Core: 8 files (4 .h, 4 .cpp)
- Hardware: 4 files (2 .h, 2 .cpp)
- Lua Bridge: 6 files (3 .h, 3 .cpp)
- Desktop: 4 files
- Teensy: 3 files
- Platform wrappers: 6 files (Swift/Obj-C++)
- Lua modes: 17 files

**Dependencies:**
- Lua 5.4 (embedded scripting)
- RtMidi (bundled, MIDI I/O)
- SDL2 (desktop GUI)
- Dear ImGui (bundled, GUI)
- FluidSynth (optional, audio synthesis)
- JSON (bundled, nlohmann/json)
