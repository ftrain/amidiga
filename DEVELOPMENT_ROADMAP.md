# GRUVBOK Development Roadmap

**Current Phase: Phase 5 - Teensy Port (95% Complete) 🚧**

This roadmap tracks GRUVBOK development progress. Phases 0-4 are complete. Teensy deployment is ready for physical hardware testing.

---

## Phase 0: Planning & Setup ✅ COMPLETE

- [x] Create README.md with project concept
- [x] Create CLAUDE.md with technical architecture
- [x] Create PROJECT_STRUCTURE.md with directory layout
- [x] Create .gitignore
- [x] Create slash commands for common tasks
- [x] Create DEVELOPMENT_ROADMAP.md

---

## Phase 1: Core Foundation (Desktop) ✅ COMPLETE

### 1.1 Data Model ✅
- [x] Event bit-packing (29 bits: 1 switch + 4×7 bit pots)
- [x] Track class (16 Events)
- [x] Pattern class (8 Tracks)
- [x] Mode class (32 Patterns)
- [x] Song class (15 Modes)
- [x] Memory footprint: 245KB

### 1.2 Build System ✅
- [x] CMake for cross-platform builds
- [x] Desktop target (macOS/Linux)
- [x] Lua 5.4 integration
- [x] RtMidi for MIDI output (bundled)
- [x] SDL2 for GUI
- [x] Dear ImGui for UI (bundled)

### 1.3 Hardware Abstraction ✅
- [x] HardwareInterface pure virtual class
- [x] DesktopHardware implementation (SDL2 + RtMidi)
- [x] MockHardware for testing

### 1.4 Playback Engine ✅
- [x] Engine class with tempo-sync'd stepping
- [x] Multi-timbral playback (15 modes simultaneously)
- [x] MIDI Clock output (24 PPQN)
- [x] LED tempo indicator
- [x] Start/Stop/Continue messages

### 1.5 MIDI Scheduler ✅
- [x] Priority queue for delta-timed events
- [x] Precise MIDI timing (<1ms jitter)
- [x] Note on/off scheduling
- [x] Control change messages

---

## Phase 2: Lua Integration ✅ COMPLETE

### 2.1 Embed Lua ✅
- [x] LuaContext class (one per mode)
- [x] Error handling and reporting
- [x] 15 contexts for 15 modes

### 2.2 Lua API ✅
- [x] `note(pitch, velocity, [delta])`
- [x] `off(pitch, [delta])`
- [x] `cc(controller, value, [delta])`
- [x] `stopall([delta])`
- [x] `led(pattern_name, [brightness])`
- [x] Internal MIDI buffer (no return value collection)

### 2.3 Mode Loader ✅
- [x] ModeLoader class
- [x] Load .lua files from `modes/` directory
- [x] Verify init() and process_event() functions
- [x] Per-mode Lua contexts with isolated state

### 2.4 Mode 0: Song Sequencer ✅
- [x] `modes/00_song.lua` - Pattern sequencer
- [x] Controls which pattern plays across all modes

### 2.5 Mode 1: Drum Machine ✅
- [x] `modes/01_drums.lua` - 808-style sequencer
- [x] 8 tracks with GM drum sounds
- [x] S1=Velocity, S2=Note Length

---

## Phase 3: Full Desktop Features ✅ COMPLETE

### 3.1 Desktop GUI ✅
- [x] Dear ImGui integration
- [x] SDL2 window management
- [x] Rotary knobs (R1-R4) with value display
- [x] Slider pots (S1-S4) with mode-specific labels
- [x] Step buttons (B1-B16) clickable grid
- [x] Pattern grid visualization
- [x] LED tempo indicator (blinks on beat)
- [x] Song Data Explorer window
- [x] System Log window

### 3.2 Global Pot Handlers ✅
- [x] R1: Mode select (0-14)
- [x] R2: Tempo (60-240 BPM)
- [x] R3: Pattern select (0-31)
- [x] R4: Track select (0-7)

### 3.3 Pattern/Track Switching ✅
- [x] Seamless pattern switching
- [x] Track isolation
- [x] Visual feedback of current position

### 3.4 Persistence ✅
- [x] JSON song format (.grv)
- [x] Song::save() and Song::load()
- [x] Sparse encoding (only active events)
- [x] Desktop GUI Save/Load buttons
- [x] Files saved to `/tmp/gruvbok_song_*.json`

### 3.5 Visual Feedback ✅
- [x] Real-time pattern grid updates
- [x] Current step indicator (red highlight)
- [x] Active events shown (green)
- [x] Mode/Pattern/Track/Step status display

---

## Phase 4: More Modes ✅ COMPLETE

### 4.1 Core Modes ✅
- [x] Mode 0: Song/Pattern Sequencer
- [x] Mode 1: Drum Machine
- [x] Mode 2: Acid Sequencer (TB-303 style)
- [x] Mode 3: Chord Sequencer (16 chord types)

### 4.2 Experimental Modes ✅
- [x] Mode 8: Drunk Sequencer (random pitch walks)
- [x] Mode 9: Cellular Automaton (Conway's Game of Life)
- [x] Mode 10: Wave Table Scanner (smooth scanning)
- [x] Mode 11: MIDI Mangler (glitch effects)
- [x] Mode 12: Lunar Phase (28-step evolution)
- [x] Mode 13: Markov Chain (probabilistic melody)
- [x] Mode 14: Tornado Spiral (Shepard tones)

### 4.3 Multi-Mode Testing ✅
- [x] All 15 modes play simultaneously
- [x] MIDI channel separation verified
- [x] No timing conflicts

---

## Phase 5: Teensy Port 🚧 95% COMPLETE

### 5.1 Teensy Hardware Layer ✅
- [x] TeensyHardware class implementing HardwareInterface
- [x] Button input (B1-B16) with debouncing
- [x] Rotary pots (R1-R4) with ADC filtering
- [x] Slider pots (S1-S4) with ADC filtering
- [x] LED control with PWM brightness
- [x] USB MIDI output (notes, CC, clock)

### 5.2 Teensy Build System ✅
- [x] PlatformIO configuration (`platformio.ini`)
- [x] Teensy 4.1 board settings
- [x] USB MIDI enabled
- [x] C++17 standard

### 5.3 Lua Library Integration ✅
- [x] Lua 5.4.6 library structure in `lib/lua/`
- [x] PlatformIO library.json with optimization flags
- [x] LUA_32BITS for memory reduction (~30-40%)
- [x] Download instructions in README

### 5.4 SD Card Support ✅
- [x] SD card initialization in main.cpp
- [x] Load Lua modes from `/modes` directory
- [x] ModeLoader integration
- [x] FAT32 compatibility

### 5.5 Hardware Testing ⏳ PENDING
- [ ] Test on physical Teensy 4.1
- [ ] Verify all 16 buttons respond
- [ ] Verify pot readings (R1-R4, S1-S4)
- [ ] Verify LED blinks on beat
- [ ] Verify USB MIDI output
- [ ] Verify SD card mode loading
- [ ] Test with DAW/synth

### 5.6 Memory Optimization ⏳ PENDING
- [ ] Profile actual RAM usage on Teensy
- [ ] Confirm <800KB usage
- [ ] Reduce modes if needed (recommend 8-12)
- [ ] Test with all modes loaded

### 5.7 LED Feedback ✅
- [x] Tempo beat blink
- [x] PWM brightness support
- [x] Pattern-based LED control (saving, loading, error, etc.)

---

## Phase 6: Testing & Documentation ✅ COMPLETE

### 6.1 Test Suite ✅
- [x] test_event.cpp (9 tests)
- [x] test_pattern.cpp (12 tests)
- [x] test_song.cpp (12 tests)
- [x] test_midi_scheduler.cpp (15 tests)
- [x] test_engine.cpp (8 tests)
- [x] **Total: 56/56 tests passing (100%)**

### 6.2 Documentation ✅
- [x] CLAUDE.md - Complete technical guide
- [x] README.md - Project overview
- [x] QUICKSTART.md - Get started guide
- [x] BUILD.md - Build instructions
- [x] docs/LUA_API.md - Lua mode development
- [x] docs/SONG_FORMAT.md - File format spec
- [x] docs/TEENSY_BUILD_GUIDE.md - Teensy build
- [x] docs/TEENSY_DEPLOYMENT_GUIDE.md - Complete deployment
- [x] Mode templates and examples

### 6.3 Example Content ✅
- [x] 15 complete Lua modes (00-14)
- [x] TEMPLATE.lua for new modes
- [x] Documented mode patterns

---

## Current Status

**Phase:** 5 (Teensy Port) - 95% Complete
**Completed:** Desktop implementation fully functional, Teensy firmware ready
**Next Task:** Test on physical Teensy 4.1 hardware
**Blockers:** None - awaiting physical hardware

## Deployment Checklist

### Desktop (macOS/Linux) ✅
- [x] Install dependencies: cmake, lua, sdl2
- [x] Build: `cmake -B build && cmake --build build`
- [x] Run: `bin/gruvbok`
- [x] Create patterns with GUI
- [x] Save/Load songs
- [x] Connect to MIDI synth

### Teensy 4.1 ⏳
- [x] Download Lua 5.4.6 source to `lib/lua/`
- [x] Prepare microSD card (FAT32, copy modes to `/modes`)
- [x] Build firmware: `platformio run -e teensy41`
- [x] Check memory: `pio run -e teensy41 -t size`
- [ ] Upload to Teensy: `platformio run -e teensy41 --target upload`
- [ ] Insert SD card
- [ ] Monitor serial: `platformio device monitor`
- [ ] Test buttons, pots, MIDI output
- [ ] Profile memory usage

---

## Summary

**Completed Phases:** 0, 1, 2, 3, 4, 6 ✅
**Current Phase:** 5 (Teensy Port) - 95% ✅
**Pending:** Physical hardware testing

GRUVBOK desktop version is production-ready. Teensy firmware is ready for deployment pending physical hardware validation.

**Memory Estimates (Teensy 4.1):**
- Event data: ~245 KB
- Lua contexts (8-12 modes): ~320-600 KB
- Code/stack: ~200-300 KB
- **Total: ~600-900 KB** (well within 1MB limit)

---

See **CLAUDE.md** for architecture details and **docs/TEENSY_DEPLOYMENT_GUIDE.md** for next steps.
