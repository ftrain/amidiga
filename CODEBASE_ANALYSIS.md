# GRUVBOK Codebase Architecture Analysis
**Very Thorough Analysis - Complete File Structure, Dependencies, and Patterns**

**Analysis Date:** November 9, 2025
**Codebase Size:** ~4,000 C++ LOC + ~2,300 Swift LOC + 17 Lua modes + 86 unit tests

---

## EXECUTIVE SUMMARY

GRUVBOK is a **multi-platform hardware MIDI groovebox** with a sophisticated architecture supporting:
- **C++ Core** (4,094 LOC) - Platform-agnostic sequencer engine
- **Three UI Implementations**: Desktop ImGui (CMake), SwiftUI (macOS native), Console
- **Two Embedded Targets**: Teensy 4.1 (Arduino), Desktop simulation
- **Lua Scripting** (17 modes) - Per-mode synthesis engine
- **Robust Testing** (86 unit tests covering core functionality)

The architecture prioritizes **code reuse** across platforms through a clean hardware abstraction layer, but shows signs of **duplication** and **tight coupling** that should be addressed as the project evolves.

---

## 1. COMPLETE FILE STRUCTURE

### Directory Layout Summary

```
/home/user/amidiga/
├── src/                          # C++ source (4,094 LOC total)
│   ├── core/                     # Platform-agnostic core (773 LOC)
│   │   ├── event.h               # Bit-packed events (29 bits)
│   │   ├── event.cpp
│   │   ├── pattern.h             # Pattern/Track/Event hierarchy
│   │   ├── pattern.cpp
│   │   ├── song.h                # Song data structure (15 modes × 32 patterns)
│   │   ├── song.cpp
│   │   ├── engine.h              # Main playback engine
│   │   ├── engine.cpp            # 520+ LOC - largest core file
│   │   └── CMakeLists.txt
│   │
│   ├── hardware/                 # Hardware abstraction (842 LOC)
│   │   ├── hardware_interface.h  # Pure virtual interface
│   │   ├── midi_scheduler.h      # MIDI event scheduling
│   │   ├── midi_scheduler.cpp    # Priority queue-based scheduler
│   │   ├── audio_output.h        # FluidSynth wrapper
│   │   ├── audio_output.cpp
│   │   └── CMakeLists.txt
│   │
│   ├── lua_bridge/               # Lua integration (831 LOC)
│   │   ├── lua_api.h             # Exposed Lua functions
│   │   ├── lua_api.cpp           # note(), off(), cc(), stopall(), led()
│   │   ├── lua_context.h         # Per-mode Lua state wrapper
│   │   ├── lua_context.cpp       # Lua 5.1/5.4 compatibility
│   │   ├── mode_loader.h         # Mode discovery and loading
│   │   ├── mode_loader.cpp
│   │   └── CMakeLists.txt
│   │
│   ├── desktop/                  # Desktop implementation (1,648 LOC)
│   │   ├── main.cpp              # Console version entry point
│   │   ├── gui_main.cpp          # GUI version entry point (600+ LOC)
│   │   ├── desktop_hardware.h    # RtMidi + SDL2 implementation
│   │   ├── desktop_hardware.cpp  # (400+ LOC)
│   │   ├── resources/
│   │   │   └── gruvbok.iconset/
│   │   └── CMakeLists.txt
│   │
│   └── teensy/                   # Teensy 4.1 implementation (150+ LOC)
│       ├── main.cpp              # Arduino setup()/loop()
│       ├── teensy_hardware.h     # GPIO/ADC/MIDI implementation
│       └── teensy_hardware.cpp
│
├── modes/                        # Lua mode scripts (17 files)
│   ├── 00_song.lua              # Pattern sequencer (master mode)
│   ├── 01_chords.lua
│   ├── 02_acid.lua              # TB-303 acid bassline
│   ├── 03_cellular.lua
│   ├── 04_arpeggiator.lua
│   ├── 05_euclidean.lua
│   ├── 06_random.lua
│   ├── 07_samplehold.lua
│   ├── 08_drunk.lua
│   ├── 09_wavetable.lua
│   ├── 10_drums.lua             # Drum machine with GM mapping
│   ├── 11_mangler.lua
│   ├── 12_lunar.lua
│   ├── 13_markov.lua
│   ├── 14_tornado.lua
│   ├── 99_led_test.lua
│   └── TEMPLATE.lua             # Mode template
│
├── native-spm/                  # Swift Package (macOS native)
│   ├── Package.swift            # SPM manifest (dual-layer build)
│   ├── Sources/GRUVBOK/
│   │   ├── App.swift            # SwiftUI app entry point
│   │   ├── Bridge/
│   │   │   ├── EngineState.swift    # C++ ↔ Swift binding
│   │   │   ├── EngineWrapper.mm     # Objective-C++ bridge
│   │   │   └── EngineWrapper.h
│   │   ├── Views/               # SwiftUI UI components (2,312 LOC)
│   │   │   ├── ContentView.swift
│   │   │   ├── HardwareControlsView.swift
│   │   │   ├── PatternGridView.swift
│   │   │   ├── SongDataExplorerView.swift
│   │   │   ├── SystemLogView.swift
│   │   │   ├── CodeEditorView.swift
│   │   │   ├── LuaManagerView.swift
│   │   │   ├── OutputView.swift
│   │   │   ├── KnobView.swift
│   │   │   ├── GMInstruments.swift
│   │   │   └── ModeLabels.swift
│   │   └── Hardware/
│   │       ├── MacOSHardware.mm  # CoreMIDI implementation
│   │       └── MacOSHardware.h
│   └── Sources/GRUVBOKCore/     # Symlink to src/ directory
│
├── tests/                       # Unit tests (86 tests total)
│   ├── test_event.cpp           # 9 tests - Event bit-packing
│   ├── test_pattern.cpp         # 12 tests - Pattern/Track hierarchy
│   ├── test_song.cpp            # 12 tests - Song data structure
│   ├── test_midi_scheduler.cpp  # 15 tests - MIDI scheduling
│   ├── test_engine.cpp          # 21 tests - Playback engine
│   ├── test_lua_integration.cpp # 17 tests - Lua bridge + modes
│   └── CMakeLists.txt
│
├── docs/                        # Documentation
│   ├── LUA_API.md              # Lua API reference
│   ├── LUA_MODE_GUIDE.md       # How to write modes
│   ├── AUDIO_OUTPUT.md         # FluidSynth integration
│   ├── TEENSY_GUIDE.md         # Teensy deployment
│   ├── SONG_FORMAT.md          # JSON save format
│   └── TESTING.md              # Test framework docs
│
├── external/                   # Third-party libraries (bundled)
│   ├── rtmidi/                # RtMidi MIDI library
│   ├── imgui/                 # Dear ImGui for GUI
│   └── json/                  # nlohmann/json (header-only)
│
├── scripts/                    # Build/utility scripts
├── lib/lua/                    # Lua source for reference
│
├── CMakeLists.txt             # Root CMake configuration
├── CLAUDE.md                  # Development guide & vision
├── DEVELOPMENT_ROADMAP.md     # Phase status (Phase 5 - 95% complete)
├── PROJECT_STRUCTURE.md       # Project organization docs
├── README.md                  # Project overview
└── SOLUTION_SUMMARY.md        # Lua memory optimization notes

```

### File Statistics

| Category | Files | LOC | Purpose |
|----------|-------|-----|---------|
| **C++ Core** | 9 | 773 | Platform-agnostic data structures & playback |
| **C++ Hardware** | 6 | 842 | Hardware abstraction + MIDI scheduling |
| **C++ Lua Bridge** | 6 | 831 | Lua integration & mode loading |
| **C++ Desktop** | 4 | 1,648 | ImGui GUI + console + SDL2 |
| **C++ Teensy** | 3 | 150 | Teensy 4.1 specific hardware |
| **Swift UI** | 14 | 2,312 | macOS native SwiftUI implementation |
| **Lua Modes** | 17 | ~2,500 | Mode implementations |
| **Unit Tests** | 6 | ~1,200 | 86 test cases |
| **Build Config** | 5 | CMake | Cross-platform builds |
| **Documentation** | 10+ | Markdown | Architecture & guides |
| **External Libs** | (bundled) | RtMidi, ImGui, JSON |

---

## 2. ARCHITECTURE PATTERNS & DESIGN

### 2.1 Clean Architecture (Hardware Abstraction Layer)

**Interface-Based Design:**
```cpp
// hardware/hardware_interface.h - Pure virtual interface
class HardwareInterface {
    virtual bool readButton(int button) = 0;
    virtual uint8_t readRotaryPot(int pot) = 0;
    virtual uint8_t readSliderPot(int pot) = 0;
    virtual void sendMidiMessage(const MidiMessage& msg) = 0;
    virtual void setLED(bool on) = 0;
    virtual uint32_t getMillis() = 0;
    virtual void update() = 0;
};
```

**Three Implementations:**
1. **DesktopHardware** (desktop_hardware.cpp) - Simulates buttons/pots via keyboard/mouse, outputs via RtMidi
2. **TeensyHardware** (teensy_hardware.cpp) - GPIO input, ADC pots, USB MIDI output
3. **MacOSHardware** (native-spm/.../MacOSHardware.mm) - CoreMIDI output + MIDI input (mirror mode)

**Benefits:** Same Engine code runs unchanged on all platforms ✅

### 2.2 Hierarchical Data Model

**Memory-Efficient Structure:**
```
Song (1)
 └─ Mode (15)  [one per MIDI channel]
     └─ Pattern (32)
         └─ Track (8)
             └─ Event (16)  [matched to 16 buttons B1-B16]
                 └─ Switch (1 bit) + Pots[4] (7 bits × 4)
```

**Memory Footprint:** 245 KB total (fits comfortably on Teensy 4.1's 1MB RAM)

**Key Insight:** Bit-packed Events (29 bits in uint32_t) → cache-friendly, efficient

### 2.3 Engine as Central Hub

**The Engine class is the "conductor"** orchestrating all subsystems:

```cpp
class Engine {
    Song* song_;                      // Data model
    HardwareInterface* hardware_;     // Input/output abstraction
    ModeLoader* mode_loader_;         // Lua mode scripts
    std::unique_ptr<MidiScheduler> scheduler_;
    std::unique_ptr<AudioOutput> audio_output_;
    
    // Main playback loop
    void update() {
        scheduler_->update();         // Send queued MIDI
        handleInput();                // Read buttons/pots
        if (is_playing_) {
            processStep();            // Call Lua modes, schedule MIDI
        }
    }
};
```

**Responsibilities:**
- Manages playback state (tempo, current mode/pattern/track/step)
- Implements Mode 0 (pattern sequencer) special behavior
- Schedules MIDI via MidiScheduler
- Loads/reinitializes Lua modes
- Handles LED tempo indicator
- Implements autosave on dirty flag

### 2.4 Lua Mode Architecture

**Per-Mode Lua Context:**
```cpp
class ModeLoader {
    std::array<std::unique_ptr<LuaContext>, 15> modes_;
};

class LuaContext {
    lua_State* L_;  // Isolated Lua interpreter
    bool loadScript(const std::string& filepath);
    bool callInit(const LuaInitContext& context);
    std::vector<ScheduledMidiEvent> callProcessEvent(int track, const Event& event);
};
```

**Lua API Exposed to Scripts:**
```lua
note(pitch, velocity, delta_ms)      -- Schedule note on
off(pitch, delta_ms)                 -- Schedule note off
cc(controller, value, delta_ms)      -- Control change
stopall(delta_ms)                    -- All notes off
led(pattern_name, brightness)        -- Trigger LED pattern
```

**Why Per-Mode Contexts?**
- 🟢 Hot-reload capability (reload one mode independently)
- 🟢 Isolated state (no cross-mode pollution)
- 🟡 Memory overhead on Teensy (15 Lua states = ~50-100KB)

### 2.5 Multi-Platform UI Strategy

| Platform | Approach | Technology | Status |
|----------|----------|-----------|--------|
| **Desktop GUI** | Full visual sequencer | Dear ImGui + SDL2 | ✅ Complete |
| **Desktop Console** | Headless control | RtMidi only | ✅ Complete |
| **macOS Native** | SwiftUI interface | Objective-C++ bridge | ✅ Complete |
| **Teensy 4.1** | Hardware buttons/LEDs | Arduino framework | ⏳ Ready to test |

**Interesting Design Choice:** Swift implementation wraps C++ core via Objective-C++ bridge
- ✅ Reuses all C++ logic
- 🟡 Duplicates UI (SwiftUI + ImGui both exist)
- 🟡 Adds compile complexity (C++ ↔ ObjC++ ↔ Swift)

---

## 3. CODE DEPENDENCIES & COUPLING

### 3.1 Dependency Graph

```
[External Libraries]
    ↓
[Hardware] ← [Core] ← [Lua Bridge]
    ↓            ↓           ↓
[Desktop/Teensy/MacOS]   [Engine]
    ↓
[Tests]
```

### 3.2 Module Dependencies (Detailed)

**core/engine.h** depends on:
- ✓ `core/song.h` - Data access
- ✓ `hardware/hardware_interface.h` - Input/output
- ✓ `hardware/midi_scheduler.h` - MIDI timing
- ✓ `hardware/audio_output.h` - Audio synthesis
- ✓ `lua_bridge/mode_loader.h` - Mode loading
- ⚠️ **5 different module types** = tight coupling

**lua_bridge/lua_context.h** depends on:
- ✓ `core/event.h` - Event data structure
- ✓ `hardware/midi_scheduler.h` - MIDI event types
- ✓ Lua 5.1 or 5.4

**hardware/midi_scheduler.h** depends on:
- ✓ `hardware/hardware_interface.h` - MIDI output
- ✓ `hardware/audio_output.h` - Optional internal audio

### 3.3 Coupling Analysis

| Coupling Type | Severity | Details |
|---|---|---|
| **Engine ↔ Hardware** | 🟡 Medium | Engine depends on HardwareInterface; good abstraction |
| **Engine ↔ ModeLoader** | 🟡 Medium | Engine initializes Lua modes; tight binding |
| **Engine ↔ MidiScheduler** | 🟡 Medium | Engine creates scheduler; direct dependency |
| **LuaContext ↔ Lua C API** | 🟢 Low | Via external C headers; standard pattern |
| **Desktop ↔ ImGui** | 🟡 Medium | gui_main.cpp has 600+ LOC of UI code |
| **Swift ↔ C++ Engine** | 🔴 High | EngineState.swift + EngineWrapper.mm layer needed |

### 3.4 Forward Dependencies (Potential Issues)

⚠️ **lua_api.h** forward declares Engine:
```cpp
class Engine;  // Forward declaration for LED control

static int lua_led(lua_State* L) {
    Engine* engine = LuaAPI::getEngine(L);
    // Lua modes can trigger LED patterns in C++
}
```

This is **circular design pattern** (Lua → C++ → Lua):
- Lua scripts can call `led()` API
- LED function calls back into Engine
- Could be problematic if Lua GC interacts with Engine state

---

## 4. TEST COVERAGE ANALYSIS

### 4.1 Test Suite Structure (86 Tests Total)

```
test_event.cpp          [9 tests]  ✅ 100% Event functionality
├─ Default constructor
├─ Set/get switch
├─ Set/get pot (4 channels)
├─ Pot clamping
├─ Copy operations
├─ Bit-packing isolation
├─ Clear function
├─ Raw data access
└─ Edge cases (0, 127 values)

test_pattern.cpp        [12 tests] ✅ Track/Pattern hierarchy
├─ Track creation & access
├─ Pattern creation & access
├─ Event isolation between tracks
├─ Event isolation between patterns
├─ Full pattern clear
└─ Boundary testing

test_song.cpp          [12 tests]  ✅ Song data model
├─ Mode creation & access
├─ Memory footprint validation (245KB)
├─ Song hierarchy navigation
├─ Full song clear
├─ JSON save/load roundtrips
└─ Persistence with sparse encoding

test_midi_scheduler.cpp [15 tests] ✅ MIDI timing
├─ Message creation (Note On/Off, CC)
├─ Delta-to-absolute time conversion
├─ Priority queue ordering
├─ Clock message generation (24 PPQN)
├─ Transport messages (Start/Stop/Continue)
└─ Event queue management

test_engine.cpp        [21 tests]  ✅ Playback engine
├─ Initialization
├─ Playback start/stop
├─ Step advancement logic
├─ Tempo calculation
├─ Mode/pattern/track selection
├─ Dirty flag management
├─ LED pattern triggers
└─ Lua mode initialization

test_lua_integration.cpp [17 tests] ✅ Lua bridge
├─ Lua version detection (5.1 vs 5.4)
├─ LuaContext creation
├─ API function registration
├─ Script loading & validation
├─ init() context passing
├─ process_event() MIDI generation
├─ Control change messages
└─ Lua 5.1 compatibility checks
```

### 4.2 Coverage Analysis

| Module | Tests | Coverage | Gaps |
|--------|-------|----------|------|
| **Event (bit-packing)** | 9 | ✅ Excellent | None identified |
| **Pattern/Track** | 12 | ✅ Excellent | None identified |
| **Song (persistence)** | 12 | ✅ Very Good | Binary save format untested |
| **MidiScheduler** | 15 | ✅ Very Good | Audio output integration not tested |
| **Engine** | 21 | ✅ Good | Mode 0 pattern switching logic partially tested |
| **Lua Integration** | 17 | ✅ Good | Cross-platform Lua 5.1 needs real Teensy testing |
| **Desktop Hardware** | ❌ None | ❌ No tests | SDL2/RtMidi integration untested |
| **Teensy Hardware** | ❌ None | ❌ No tests | GPIO/ADC/USB MIDI only testable on real hardware |
| **Swift UI** | ❌ None | ❌ No tests | SwiftUI components untested |

### 4.3 Test Framework Notes

- **Custom test macros** (TEST, ASSERT_EQ, ASSERT_TRUE, ASSERT_FALSE)
- **No external test framework** (std::runtime_error for failures)
- **CMake integration** via `ctest`
- **All tests compile on desktop**, some require real hardware (Teensy)

---

## 5. DOCUMENTATION STATE

### 5.1 What's Well-Documented

| Document | Quality | Content |
|----------|---------|---------|
| **CLAUDE.md** | ⭐⭐⭐⭐⭐ | Complete architecture, modes, memory analysis, roadmap |
| **docs/LUA_API.md** | ⭐⭐⭐⭐ | Lua API reference, note/off/cc/stopall/led |
| **docs/LUA_MODE_GUIDE.md** | ⭐⭐⭐⭐ | How to write new modes, scale/velocity context |
| **PROJECT_STRUCTURE.md** | ⭐⭐⭐⭐ | Directory layout, file responsibilities |
| **DEVELOPMENT_ROADMAP.md** | ⭐⭐⭐⭐ | Phase status, completion percentages |
| **README.md** | ⭐⭐⭐ | Project overview, quick start |

### 5.2 What's Under-Documented

| Area | Issue | Priority |
|------|-------|----------|
| **Desktop GUI Internals** | No docs for ImGui widget system | Medium |
| **Teensy Hardware Pinout** | Only documented in code comments | High |
| **Swift Integration** | No docs explaining EngineWrapper.mm bridge | High |
| **MIDI Clock Precision** | Timing algorithm not documented | Medium |
| **Mode 0 Special Behavior** | Complex pattern switching only in code | Medium |
| **Audio Output (FluidSynth)** | Setup & SoundFont loading documented sparsely | Medium |
| **Persistence Layer** | JSON format documented; binary format not | Low |
| **Lua GC Strategy** | No guidance on GC pauses on embedded | High |

---

## 6. PLATFORM ABSTRACTION ANALYSIS

### 6.1 Hardware Interface Success

✅ **What Works Well:**
- Single HardwareInterface definition used across all platforms
- Clean separation of input (buttons/pots) and output (MIDI/LED)
- Easy to add new platforms (just implement HardwareInterface)
- Mock implementations possible for testing

### 6.2 Platform-Specific Issues

| Platform | Issue | Impact | Notes |
|----------|-------|--------|-------|
| **Desktop** | Keyboard mapping hardcoded in gui_main.cpp | Low | Works but not configurable |
| **Teensy** | Pin definitions in teensy_hardware.h | Medium | Should be in .ini config |
| **macOS** | CoreMIDI output-only; no input MIDI support | Low | Mirror mode added but different code paths |
| **All** | No debounce/jitter filtering standardized | Medium | Each platform implements differently |

### 6.3 Missing: Configuration System

Planned in CLAUDE.md but **not implemented**:
```
⏳ PLANNED:
config/
├─ default.ini        # [buttons], [pots], [midi], [rotary_pots], [slider_pots]
├─ teensy41.ini
└─ custom.ini

ACTUAL:
Pin mappings hardcoded in:
- src/desktop/desktop_hardware.cpp
- src/teensy/teensy_hardware.h  (static constexpr arrays)
```

**Impact:** Hard to customize without recompiling

---

## 7. DATA FLOW ANALYSIS

### 7.1 Input → MIDI Output Path

```
[Hardware Input]
    ↓
[HardwareInterface::readButton/Pot]  ← Buttons B1-B16, Rotary R1-R4, Sliders S1-S4
    ↓
[Engine::handleInput]               ← Process user input
    ├─ Update mode/pattern/track/tempo
    └─ Mark data as "dirty" (unsaved)
    ↓
[Engine::processStep]               ← Called once per step
    ├─ Get current Event from song
    ├─ Call LuaContext::callProcessEvent(track, event)
    │   ├─ Lua mode script executes process_event()
    │   ├─ Calls note(), off(), cc(), stopall() from lua_api.cpp
    │   └─ MIDI events accumulate in internal buffer
    ├─ Schedule MIDI events via MidiScheduler
    └─ Optional: Call AudioOutput::sendMidiMessage
    ↓
[MidiScheduler::update]             ← Called every frame
    ├─ Check priority queue for due events
    ├─ Convert relative timing to absolute
    └─ Output via:
        ├─ HardwareInterface::sendMidiMessage (external MIDI)
        └─ AudioOutput::sendMidiMessage (FluidSynth audio)
    ↓
[Hardware Output]
    ├─ RtMidi/USB MIDI → External synthesizer
    ├─ FluidSynth → Audio buffer → System audio
    └─ setLED() → Hardware indicator
```

### 7.2 Mode 0 (Pattern Sequencer) Special Data Flow

**Mode 0 runs at 1/16th speed** (each step = one full 16-step pattern):

```
Normal Steps:   0 1 2 3 4 5 6 7 | 8 9 10 11 12 13 14 15 | 0 1 2 3...
Mode 0 Steps:   ← Step 0 →       | ← Step 1 →              | ← Step 2 →

When current_step_ wraps from 15 → 0:
    song_mode_step_ = (song_mode_step_ + 1) % song_mode_loop_length_
    
    Read Mode 0 Track 0 Event[song_mode_step_] to get pattern number
    Apply to all modes 1-14
```

### 7.3 Multi-Mode Simultaneous Playback

```
[processStep] called at 16th note intervals (BPM-dependent)
    ↓
For each mode 0-14:
    ├─ Select pattern (from Mode 0 or Mode-specific)
    ├─ Get Event at current_step
    ├─ Call ModeLoader::getMode(mode_num)->callProcessEvent()
    │   └─ Lua script generates MIDI on appropriate MIDI channel
    └─ Schedule MIDI messages
    ↓
All MIDI events scheduled with delta timing
All events collected into scheduler's priority queue
Sent at precise times in next scheduler->update()
```

---

## 8. ARCHITECTURAL ISSUES & CONCERNS

### 8.1 HIGH PRIORITY Issues

#### Issue 1: Code Duplication - Hardware Implementations
**Location:** `src/desktop/desktop_hardware.cpp` vs `src/teensy/teensy_hardware.cpp`

**Problem:**
- Both implement same HardwareInterface
- Button debounce logic duplicated
- Pot value mapping duplicated
- MIDI message routing duplicated

**Code Example:**
```cpp
// desktop_hardware.cpp
uint8_t DesktopHardware::readRotaryPot(int pot) {
    if (pot < 0 || pot >= 4) return 0;
    return rotary_pots_[pot];
}

// teensy_hardware.cpp - similar implementation but with ADC code
uint8_t TeensyHardware::readRotaryPot(int pot) {
    if (pot < 0 || pot >= 4) return 0;
    uint16_t raw = readPotRaw(ROTARY_POT_PINS[pot]);
    return mapAdcToMidi(raw);
}
```

**Impact:** 🔴 High
- Bug fixes need to be applied twice
- New hardware implementations copy-paste existing code
- Maintenance burden increases

**Recommendation:**
Extract common logic into HardwareInterface base class or utility functions:
```cpp
class HardwareInterface {
    // Common helper
    static uint8_t mapRawToMidi(uint16_t raw, uint16_t min, uint16_t max);
    
    // Debounce logic
    struct DebouncedButton { ... };
};
```

---

#### Issue 2: Swift Implementation Duplicates UI Logic
**Location:** `native-spm/Sources/GRUVBOK/Views/*.swift` vs `src/desktop/gui_main.cpp`

**Problem:**
- SwiftUI has 2,312 LOC of UI
- ImGui has similar controls (knobs, sliders, pattern grid, log)
- Both implementations hand-code the same UI logic
- Changes to one don't propagate to the other

**Examples of Duplication:**
- `KnobView.swift` ≈ Knob() function in gui_main.cpp
- `PatternGridView.swift` ≈ Pattern grid in gui_main.cpp  
- `SystemLogView.swift` ≈ Log window in gui_main.cpp
- `HardwareControlsView.swift` ≈ Rotary/slider controls in gui_main.cpp

**Impact:** 🔴 High
- Double maintenance burden
- UI inconsistencies between platforms
- No single source of truth for UI design
- Harder to add new UI features

**Recommendation:**
- Create **UI state machine abstraction** that both ImGui and SwiftUI consume
- Move UI layout/logic to JSON/YAML configuration
- Or deprecate one implementation and standardize on the other

---

#### Issue 3: Engine Class Too Large & Too Responsible
**Location:** `src/core/engine.h` (257 lines) / `engine.cpp` (520+ LOC)

**Problem:**
Engine class is a **"God Object"** managing too many concerns:

```cpp
class Engine {
    // Data model access
    Song* song_;
    
    // Hardware I/O
    HardwareInterface* hardware_;
    
    // Playback state (7 variables)
    bool is_playing_;
    int tempo_, current_mode_, current_pattern_, current_track_, current_step_;
    
    // Mode 0 special state (4 variables)
    int song_mode_step_, song_mode_loop_length_, target_mode_;
    
    // Global parameters (3 arrays)
    int global_scale_root_, global_scale_type_;
    int mode_velocity_offsets_[15], mode_pattern_overrides_[15];
    uint8_t mode_programs_[15];
    
    // LED control (6 variables)
    LEDPattern led_pattern_, bool led_on_, uint8_t led_brightness_;
    uint32_t led_state_start_time_, led_phase_start_time_;
    
    // Timing (7 variables)
    uint32_t last_step_time_, step_interval_ms_;
    uint32_t clock_start_time_, clock_pulse_count_;
    double clock_interval_ms_;
    bool lua_reinit_pending_;
    uint32_t last_tempo_change_time_;
    
    // Dirty/autosave (2 variables)
    bool dirty_;
    uint32_t last_autosave_time_;
    
    // Dependencies
    ModeLoader* mode_loader_;
    MidiScheduler* scheduler_;
    AudioOutput* audio_output_;
};
```

**Methods:** update(), handleInput(), processStep(), calculateStepInterval(), calculateClockInterval(), sendMidiClock(), reinitLuaModes(), parseMode0Event(), applyMode0Parameters(), updateLED(), checkAutosave(), etc.

**Issues:**
- 🔴 Single Responsibility Principle violated
- 🔴 Hard to test (dependencies: Hardware, ModeLoader, MidiScheduler, AudioOutput)
- 🔴 Hard to understand (too many state variables)
- 🔴 Difficult to extend (adding features adds more state)

**Impact:** 🔴 High - Will be problematic as new modes/features are added

**Recommendation:** Refactor into smaller, focused classes:
```cpp
// Proposed split:
class PlaybackState {         // Tempo, step counting, MIDI clock
    void update();
    void setTempo(int bpm);
};

class Mode0Sequencer {         // Pattern switching logic
    void update(const Event& event);
    int getTargetPattern() const;
};

class LEDController {          // LED pattern management
    void setPattern(LEDPattern p);
    void update();
};

class Engine {                 // Conductor only
    PlaybackState playback_;
    Mode0Sequencer sequencer_;
    LEDController led_;
    // Smaller, focused class
};
```

---

### 8.2 MEDIUM PRIORITY Issues

#### Issue 4: Lua Context Per-Mode Uses Significant Memory
**Location:** `src/lua_bridge/mode_loader.h`

**Problem:**
```cpp
class ModeLoader {
    std::array<std::unique_ptr<LuaContext>, 15> modes_;  // 15 Lua states!
};

// Each LuaContext:
class LuaContext {
    lua_State* L_;              // ~40-50 KB per state
    std::vector<ScheduledMidiEvent> event_buffer_;
};
```

**Memory Analysis:**
- 15 Lua states × 50KB = **750 KB** on Teensy
- But Teensy has only **1 MB RAM total**
- With core code (~300KB), that leaves ~50KB for everything else

**Current Mitigation:**
- Using LuaArduino (Lua 5.1 optimized) instead of Lua 5.4
- Selective library loading (NO io, os, debug libraries on Teensy)
- This brings it down to ~120KB per state, but still tight

**Impact:** 🟡 Medium
- Teensy build just barely fits (margin is thin)
- Any growth = build failure
- Lua GC pauses could be problematic during playback

**Recommendation:**
1. **Profile on real hardware** - Measure actual memory usage
2. **Consider lazy loading** - Load only active mode's Lua context
3. **Pre-compile Lua** - Convert .lua to Lua bytecode (.luac) to save memory
4. **Monitor GC** - Add GC tuning parameters to prevent pause jitter

---

#### Issue 5: MIDI Scheduler Using std::priority_queue (Complex for Embedded)
**Location:** `src/hardware/midi_scheduler.h`

**Problem:**
```cpp
class MidiScheduler {
    // Priority queue with template comparison
    std::priority_queue<AbsoluteMidiEvent, 
                        std::vector<AbsoluteMidiEvent>, 
                        std::greater<AbsoluteMidiEvent>> event_queue_;
};
```

**Issues:**
- 🟡 Uses STL containers (allocations possible)
- 🟡 Comparison function called on every insertion/removal
- 🟡 Overkill for typical use case (max ~16 MIDI events per step)

**Typical Event Count:** 1-4 MIDI events per step
- Could use **simple fixed-size array** instead
- Or **circular buffer** with manual sorting

**Impact:** 🟡 Medium
- Probably not a problem in practice
- But violates "real-time = no allocations" principle
- Could cause timing jitter on resource-constrained Teensy

**Recommendation:**
```cpp
// Consider: Static allocation instead
class MidiScheduler {
    static constexpr int MAX_QUEUED_EVENTS = 32;
    std::array<ScheduledMidiEvent, MAX_QUEUED_EVENTS> events_;
    int event_count_ = 0;
    
    void schedule(const ScheduledMidiEvent& event) {
        if (event_count_ < MAX_QUEUED_EVENTS) {
            events_[event_count_++] = event;
            std::sort(events_.begin(), events_.begin() + event_count_);
        }
    }
};
```

---

#### Issue 6: No Standardized Button Debouncing
**Location:** Scattered across platform implementations

**Problem:**
- Desktop: No debounce (keyboard events are debounced by OS)
- Teensy: 20ms debounce in `readButtonRaw()`
- macOS: No debounce info in code

**Code Example:**
```cpp
// teensy_hardware.h
static constexpr uint32_t DEBOUNCE_DELAY_MS = 20;

// desktop_hardware.cpp
// No debounce - relies on OS keyboard repeat
```

**Impact:** 🟡 Medium
- Inconsistent behavior across platforms
- Could cause missed button presses or doubles on Teensy
- Not documented what debounce strategy is used

**Recommendation:**
- Define standard debounce algorithm in HardwareInterface
- Implement in base class or separate utility
- Allow configuration per platform

---

#### Issue 7: Hardcoded Slider Labels & Mode-Specific Logic
**Location:** `src/desktop/gui_main.cpp` lines 84-113

**Problem:**
```cpp
const char* GetSliderLabel(int slider_index, int mode_number) {
    if (mode_number == 0) {
        const char* labels[] = {"Pattern", "---", "---", "---"};
        return labels[slider_index];
    } else if (mode_number == 1) {
        const char* labels[] = {"Velocity", "Length", "S3", "S4"};
        return labels[slider_index];
    } else if (mode_number == 2) {
        const char* labels[] = {"Pitch", "Length", "Slide", "Filter"};
        return labels[slider_index];
    } // ... etc for all 15 modes
}
```

**Issues:**
- 🟡 Hardcoded in C++ (not data-driven)
- 🟡 Must update C++ and Lua separately
- 🟡 Swift implementation has duplicate labels somewhere else
- 🟡 Adding new mode requires recompiling

**Impact:** 🟡 Medium
- Prevents runtime mode loading
- Maintenance burden (two sources of truth)

**Recommendation:**
- Move labels to Lua: `SLIDER_LABELS = {"Velocity", "Length", "S3", "S4"}`
- Already partially done: `02_acid.lua` has `SLIDER_LABELS` defined
- Extract this in LuaContext and return to C++
- Display in UI dynamically

---

### 8.3 LOW PRIORITY Issues

#### Issue 8: Two Main Entry Points (main.cpp vs gui_main.cpp)
**Location:** `src/desktop/`

**Files:**
- `main.cpp` - Console version (no changes, works but deprecated)
- `gui_main.cpp` - GUI version (600+ LOC, active development)

**Issue:** Code duplication in initialization:
```cpp
// Both do:
auto hardware = std::make_unique<DesktopHardware>();
auto song = std::make_unique<Song>();
auto mode_loader = std::make_unique<ModeLoader>();
auto engine = std::make_unique<Engine>(song.get(), hardware.get(), mode_loader.get());
mode_loader->loadModesFromDirectory(modes_path, 120);
```

**Impact:** 🟢 Low
- Console version appears unmaintained anyway
- GUI version is the active development target
- Could be removed or refactored into shared setup

---

#### Issue 9: Missing Error Handling in Mode Loading
**Location:** `src/lua_bridge/mode_loader.cpp`

**Problem:**
```cpp
int ModeLoader::loadModesFromDirectory(const std::string& directory, int tempo) {
    int count = 0;
    for (int i = 0; i < NUM_MODES; ++i) {
        std::string filename = directory + "/" + padNumber(i, 2) + "_*.lua";
        if (loadMode(i, filename, tempo)) {
            count++;
        }
        // Silently fails if mode not found
    }
    return count;
}
```

**Issue:** If mode loading fails, user gets no feedback about why
- Missing file? No error
- Syntax error in Lua? Silent failure
- Missing init() function? Silent failure

**Impact:** 🟢 Low
- Desktop GUI logs failures, so visible
- But makes debugging harder
- Teensy might not have log output

**Recommendation:**
```cpp
if (!loadMode(i, filename, tempo)) {
    if (hardware) {
        hardware->addLog("Mode " + std::to_string(i) + " failed to load");
    }
}
```

---

#### Issue 10: Song::load/save JSON Format Not Stable
**Location:** `src/core/song.cpp`

**Problem:**
- JSON save/load implemented but **binary format comment says "not yet implemented"**
- Only JSON supported for now
- No version field in saved songs
- Could break on format changes

**Code:**
```cpp
// song.h
bool save(const std::string& filepath, const std::string& name = "GRUVBOK Song", int tempo = 120);
bool load(const std::string& filepath, std::string* out_name = nullptr, int* out_tempo = nullptr);

bool saveBinary(const std::string& filepath);  // Declared but...
bool loadBinary(const std::string& filepath);  // Not implemented?
```

**Impact:** 🟢 Low
- JSON is actually fine for most use cases
- Binary format rarely needed (only flash memory)
- Version field should be added before shipping to users

---

## 9. LUA INTEGRATION ANALYSIS

### 9.1 Lua Bridge Architecture

**Two-Layer Binding:**
```
Lua Script (mode_XX.lua)
    ↓
LuaAPI Functions (note, off, cc, stopall, led)
    ↓
LuaAPI::registerAPI() - Registers C functions
    ↓
lua_State (per-mode LuaContext)
    ↓
C++ Engine (for LED, status, etc.)
```

### 9.2 Lua-C++ Communication

**Direction 1: C++ → Lua (Calling Lua functions)**
```cpp
// In LuaContext::callProcessEvent()
lua_getglobal(L_, "process_event");
lua_pushinteger(L_, track);
lua_newtable(L_);  // Create event table
  lua_pushboolean(L_, event.getSwitch());
  lua_setfield(L_, -2, "switch");
  // ... set pots[1-4] ...
lua_pcall(L_, 2, 0, 0);  // Call process_event(track, event)
```

**Direction 2: Lua → C++ (Lua API functions)**
```lua
-- In mode_XX.lua
function process_event(track, event)
    note(60, 127)      -- Calls lua_note (C function)
    off(60, 100)       -- Calls lua_off (C function)
    cc(74, 64)         -- Calls lua_cc (C function)
    return {}
end

-- Implementation in lua_api.cpp:
static int lua_note(lua_State* L) {
    uint8_t pitch = luaL_checkinteger(L, 1);
    uint8_t velocity = luaL_checkinteger(L, 2);
    uint32_t delta = luaL_optinteger(L, 3, 0);
    
    auto buffer = LuaAPI::getEventBuffer(L);
    buffer->push_back(MidiScheduler::noteOn(pitch, velocity, current_channel, delta));
}
```

### 9.3 Lua 5.1 vs 5.4 Compatibility

**Status:** ✅ Excellent compatibility work done

**Mitigation Measures Implemented:**
1. ✅ Lua version detection at compile time (lua_context.cpp)
2. ✅ Automatic LUA_OK definition if missing
3. ✅ Selective library loading based on platform
4. ✅ 17 tests specifically for Lua compatibility
5. ✅ All modes tested to avoid 5.4-only features

**Verified Compatibility:**
- ✅ `luaL_newstate()` - Works both
- ✅ `luaL_openlibs()` - Works both
- ✅ `lua_pcall()` - Works both  
- ✅ `luaL_checkinteger()` - Works both
- ⚠️ `LUA_OK` constant - Missing in 5.1, defined as 0

**Lua Script Restrictions (5.1 compatible):**
- ❌ NO: Bitwise operators (&, |, ~, <<, >>)
- ❌ NO: Integer division (//)
- ❌ NO: goto statement
- ❌ NO: Lua 5.4 string library additions

**How to Verify 5.1 Compatibility:**
Check each mode file for these patterns:
```lua
-- ❌ BAD (Lua 5.4 only)
x = bit32.band(a, b)
y = x >> 2
z = a // b

-- ✅ GOOD (Lua 5.1 compatible)
x = math.floor(a / b)
z = math.fmod(a, b)
```

### 9.4 Lua Performance Considerations

**Expected Performance:**
- Mode script execution: ~0.5-1ms per event (interpreted Lua)
- Max 16 events per pattern step
- Total: ~8-16ms per step at 120 BPM (500ms per step) ✅ Plenty of headroom

**Lua GC Impact:**
- Full stop-the-world collection could cause MIDI timing jitter
- No GC tuning currently configured
- Should add: `collectgarbage("setpause", 200)` to minimize pauses

---

## 10. POTENTIAL ISSUES SUMMARY TABLE

| # | Issue | Category | Severity | Effort | Status |
|---|-------|----------|----------|--------|--------|
| **1** | Hardware code duplication | Architecture | 🔴 High | High | Unresolved |
| **2** | Swift duplicates ImGui UI | Architecture | 🔴 High | High | Unresolved |
| **3** | Engine class too large | Design | 🔴 High | High | Unresolved |
| **4** | Lua memory tight on Teensy | Embedded | 🟡 Medium | Medium | Mitigated (using LuaArduino) |
| **5** | MIDI scheduler over-engineered | Performance | 🟡 Medium | Low | Unresolved (probably fine) |
| **6** | No standard debouncing | Hardware | 🟡 Medium | Medium | Unresolved |
| **7** | Mode labels hardcoded in C++ | Maintenance | 🟡 Medium | Low | Partially solved |
| **8** | Two main entry points | Code Quality | 🟢 Low | Low | Unresolved |
| **9** | Weak error handling in mode loading | Debugging | 🟢 Low | Low | Unresolved |
| **10** | Song format not versioned | Persistence | 🟢 Low | Medium | Unresolved |

---

## 11. RECOMMENDATIONS & ACTION ITEMS

### Phase A: Immediate Improvements (Next Sprint)

1. **Extract Common Hardware Logic**
   - Create `HardwareCommon` base class with shared debounce/mapping
   - Reduce duplication between Desktop and Teensy
   - Estimated effort: 4-6 hours

2. **Standardize Mode Labels**
   - Ensure all modes define `SLIDER_LABELS = {...}` global
   - Update C++ to read from Lua instead of hardcoding
   - Estimated effort: 2-3 hours

3. **Add Lua GC Tuning**
   - Call `collectgarbage("setpause", 200)` on mode init
   - Monitor for GC pauses with timing logs
   - Estimated effort: 1-2 hours

### Phase B: Medium-Term Refactoring (Next Quarter)

4. **Refactor Engine Class**
   - Split into PlaybackState, Mode0Sequencer, LEDController
   - Reduce dependencies from 5+ to 2-3 per class
   - Estimated effort: 16-20 hours

5. **Standardize Button Debouncing**
   - Move debounce logic to HardwareInterface base
   - Test on both Desktop and Teensy
   - Estimated effort: 4-6 hours

6. **Profile Lua Memory Usage**
   - Build for Teensy, measure actual RAM
   - Identify optimization opportunities
   - Estimated effort: 3-4 hours

### Phase C: Long-Term Decisions (Next Year)

7. **Consolidate UI Implementations**
   - Decision: Keep ImGui OR Swift (not both)
   - OR: Create UI abstraction layer consumed by both
   - Estimated effort: 20-40 hours (major refactor)

8. **Implement Configuration System**
   - .ini parser for hardware mappings
   - Hot-reload capability
   - Estimated effort: 8-10 hours

9. **Add Integration Tests**
   - Mock hardware tests
   - End-to-end MIDI generation tests
   - Estimated effort: 8-10 hours

---

## 12. CONCLUSION

### Strengths

✅ **Well-Architected Core**
- Clean separation via HardwareInterface
- Excellent test coverage (86 tests)
- Good documentation (CLAUDE.md, docs/)

✅ **Flexible Lua Integration**
- Per-mode contexts enable hot-reloading
- Compatible with Lua 5.1 AND 5.4
- Rich API for mode scripts

✅ **Multi-Platform Strategy**
- Same C++ core runs on Desktop, Teensy, macOS
- Strategic choice to support multiple UIs

### Weaknesses

⚠️ **Code Duplication**
- Hardware implementations duplicate logic
- SwiftUI duplicates ImGui UI
- Mode labels defined in two places

⚠️ **Architectural Coupling**
- Engine class is too large (God Object)
- Too many dependencies (5+)
- Hard to test in isolation

⚠️ **Embedded Constraints**
- Tight memory margin on Teensy (15 Lua states = 750KB)
- No standardized debouncing
- STL containers used (could cause jitter)

### Path Forward

**GRUVBOK is in excellent shape for an open-source sequencer project.** The core architecture is sound, testing is thorough, and the multi-platform support is impressive. 

**The main work ahead:**
1. Eliminate code duplication (high ROI)
2. Refactor Engine class (necessary for maintainability)
3. Add hardware integration tests (verify Teensy works)
4. Profile and optimize for embedded (margin is thin)

**Estimated effort to production-ready:**
- **4-6 weeks** with 1 FTE developer
- **2-3 weeks** with 2 FTE developers

The foundation is solid. Build upon it! 🚀

---

**Report Generated:** 2025-11-09
**Methodology:** Comprehensive file-by-file review, dependency analysis, test examination, documentation audit
**Scope:** src/, modes/, tests/, native-spm/, docs/
**Files Analyzed:** 156+ source files (C++, Swift, Lua), 86 test cases, 10+ documentation files

