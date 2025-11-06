# GRUVBOK Development Roadmap

**Current Phase: Phase 0 - Planning ✅ COMPLETE**

This roadmap breaks down GRUVBOK development into concrete, actionable tasks. Each phase builds on the previous one. Complete phases in order for best results.

---

## Phase 0: Planning & Setup ✅

**Goal: Prepare project for development**

- [x] Create README.md with project concept
- [x] Create CLAUDE.md with technical architecture
- [x] Create PROJECT_STRUCTURE.md with directory layout
- [x] Create .gitignore
- [x] Create slash commands for common tasks
- [x] Create DEVELOPMENT_ROADMAP.md (this file)

**Status: COMPLETE** ✅

---

## Phase 1: Core Foundation (Desktop)

**Goal: Build data model and basic playback engine**

### 1.1 Data Model (Priority: HIGH)

**Build the core data structures that represent musical content.**

- [ ] Create `src/core/event.h` and `src/core/event.cpp`
  - Define Event struct: 1 switch + 4 pot values
  - Implement bit-packing: 1 bit + 4×7 bits = 29 bits (fits in uint32_t)
  - Write pack() and unpack() methods
  - Add getSwitch(), getPot(n), setSwitch(), setPot(n, value)
  - Unit tests for bit manipulation

- [ ] Create `src/core/pattern.h` and `src/core/pattern.cpp`
  - Track class: array of 16 Events
  - Pattern class: array of 8 Tracks
  - Methods: getEvent(track, step), setEvent(track, step, event)
  - Clear() method to reset patterns

- [ ] Create `src/core/song.h` and `src/core/song.cpp`
  - Mode class: array of 32 Patterns
  - Song class: array of 15 Modes
  - Methods: getPattern(mode, pattern_num), setPattern(...)
  - Calculate total memory footprint
  - Add serialize/deserialize stubs (for later persistence)

**Validation:** Data structures compile, tests pass, memory is efficiently packed.

### 1.2 Build System (Priority: HIGH)

**Set up CMake for desktop builds.**

- [ ] Create root `CMakeLists.txt`
  - Support desktop target
  - Find/link Lua library
  - Find/link MIDI library (RtMidi or JUCE)
  - Set C++17 standard
  - Enable warnings (-Wall -Wextra)

- [ ] Create `src/core/CMakeLists.txt`
  - Build core library (static)

- [ ] Create `src/desktop/CMakeLists.txt`
  - Build desktop executable
  - Link core library

- [ ] Build and verify
  - `mkdir build && cd build && cmake .. && make`
  - Should compile without errors

**Validation:** Project builds successfully on desktop.

### 1.3 Hardware Abstraction (Priority: MEDIUM)

**Create interface that works for both desktop and Teensy.**

- [ ] Create `src/hardware/hardware_interface.h`
  - Define pure virtual HardwareInterface class
  - Methods: init(), readButton(n), readPot(n), sendMidi(...), setLED(state), getMillis()

- [ ] Create `src/desktop/desktop_hardware.h/cpp`
  - Implement HardwareInterface for desktop
  - Map keyboard keys to B1-B16 (e.g., QWERTY rows)
  - Use mouse/UI sliders for R1-R4, S1-S4
  - Use RtMidi for MIDI output
  - Print MIDI messages to console (debug mode)

**Validation:** Can read keyboard input, output MIDI to virtual port.

### 1.4 Basic Playback Engine (Priority: MEDIUM)

**Create engine that loops through Events.**

- [ ] Create `src/core/engine.h/cpp`
  - Engine class
  - Methods: init(song, hardware), update(), start(), stop()
  - State: current mode, pattern, track, step
  - Loop through Events at specified tempo
  - Call placeholder for Lua (just print Event for now)

- [ ] Integrate with desktop
  - Create `src/desktop/main.cpp`
  - Initialize Song, HardwareInterface, Engine
  - Run update() loop
  - Press keys to toggle Events, hear tempo tick

**Validation:** Engine loops through data, tempo is accurate, keyboard changes Events.

### 1.5 MIDI Scheduler (Priority: LOW - can defer)

**Schedule MIDI events with delta timing.**

- [ ] Create `src/hardware/midi_scheduler.h/cpp`
  - Priority queue of scheduled MIDI events
  - Methods: schedule(midi_event, delta_ms), update()
  - Send MIDI at precise times
  - Handle note-off scheduling

**Validation:** Notes play at correct times, note-off works, no jitter.

---

## Phase 2: Lua Integration

**Goal: Enable Lua modes to transform Events into MIDI**

### 2.1 Embed Lua (Priority: HIGH)

**Integrate Lua interpreter into engine.**

- [ ] Create `src/lua_bridge/lua_context.h/cpp`
  - LuaContext class wraps lua_State
  - Methods: init(), loadScript(path), callInit(), callProcessEvent()
  - Error handling and reporting
  - One context per mode

- [ ] Update CMakeLists.txt
  - Find Lua library
  - Link to lua_bridge and core

**Validation:** Can load and execute simple Lua script.

### 2.2 Lua API (Priority: HIGH)

**Expose C++ functions to Lua.**

- [ ] Create `src/lua_bridge/lua_api.h/cpp`
  - Implement `note(pitch, velocity, delta=0)`
  - Implement `off(pitch, delta=0)`
  - Implement `cc(controller, value, delta=0)`
  - Implement `stopall(delta=0)`
  - Register functions in Lua global scope
  - Return MidiEvent objects that scheduler can use

- [ ] Test from Lua
  - Write simple Lua script that calls these functions
  - Verify MIDI events are generated

**Validation:** Lua can call C++ functions, MIDI events are created.

### 2.3 Mode Loader (Priority: MEDIUM)

**Load .lua mode files and validate them.**

- [ ] Create `src/lua_bridge/mode_loader.h/cpp`
  - ModeLoader class
  - loadMode(mode_number, filepath)
  - Verify init() and process_event() functions exist
  - Handle errors gracefully

- [ ] Integrate with Engine
  - Engine loads modes at startup
  - Calls init() once per mode
  - Calls process_event(track, event) during playback

**Validation:** Modes load from files, functions are callable.

### 2.4 First Mode: Boot (Mode 0) (Priority: HIGH)

**Implement basic boot mode for load/save.**

- [ ] Create `modes/00_boot.lua`
  - init() function (empty for now)
  - process_event() function
  - Detect tap patterns on buttons
  - Stub load/save/erase (print messages)
  - Blink LED (call hardware function)

- [ ] Test on desktop
  - Tap B1 once → "Load" message
  - Tap B1 twice → "Save" message
  - Tap once + long tap → "Erase" message

**Validation:** Boot mode responds to button patterns, messages print.

### 2.5 Second Mode: Drum Machine (Mode 1) (Priority: MEDIUM)

**Implement 808-style drum sequencer.**

- [ ] Create `modes/01_drums.lua`
  - Define drum_map table (8 MIDI notes for 8 tracks)
  - process_event(): if switch on, play drum note
  - Use S1 for velocity control
  - Track 8 = accent (boosts velocity)

- [ ] Test on desktop
  - Program beats by pressing buttons
  - Hear drum sounds on MIDI channel 1
  - Adjust velocity with S1

**Validation:** Can program and play drum patterns, velocity responds to slider.

---

## Phase 3: Full Desktop Features

**Goal: Complete desktop app with all OS layer features**

### 3.1 Configuration System (Priority: MEDIUM)

- [ ] Create `src/hardware/config.h/cpp`
  - Parse .ini files
  - Map button names (B1-B16) to pins
  - Map pot names (R1-R4, S1-S4) to analog pins
  - Support comments with #

- [ ] Create `config/default.ini`
  - Desktop: map to keyboard keys
  - Define MIDI settings

- [ ] Create `config/teensy41.ini`
  - Teensy: map to actual GPIO pins
  - Define analog input pins

**Validation:** Config files load, hardware mappings apply correctly.

### 3.2 Global Pot Handlers (Priority: HIGH)

**Implement R1-R4 rotary pot functions.**

- [ ] Add to Engine
  - R1: Mode switch (0-15)
  - R2: Tempo (0-1000 BPM)
  - R3: Pattern select (1-32)
  - R4: Track select (1-8)

- [ ] Update UI/display
  - Show current mode, tempo, pattern, track
  - Update on pot changes

**Validation:** Twisting pots changes modes/tempo/pattern/track, display updates.

### 3.3 Pattern/Track Switching (Priority: HIGH)

- [ ] Implement in Engine
  - Switch patterns seamlessly (at bar boundary)
  - Switch tracks for editing
  - Visual indicator of active track

- [ ] Test
  - Program different patterns
  - Switch between them while playing
  - Verify smooth transitions

**Validation:** Can switch patterns without glitches, tracks are distinct.

### 3.4 Persistence (Priority: MEDIUM)

**Save and load songs.**

- [ ] Design file format
  - Binary: efficient, small
  - Or JSON: human-readable, debuggable
  - Include: all 15 modes, all patterns, metadata

- [ ] Implement in Song class
  - save(filepath)
  - load(filepath)

- [ ] Integrate with Boot mode
  - Update `modes/00_boot.lua`
  - Tap once: call load()
  - Tap twice: call save()
  - Tap + long: clear song

**Validation:** Songs save and load correctly, data is preserved.

### 3.5 Visual Feedback (Priority: LOW)

**Optional UI for desktop.**

- [ ] Create `src/desktop/ui/main_window.cpp`
  - Show 16 buttons (B1-B16) with on/off state
  - Show 8 sliders (R1-R4, S1-S4) with values
  - Display current: mode, tempo, pattern, track, step
  - Visualize playback position

**Validation:** UI reflects hardware state, updates in real-time.

---

## Phase 4: More Modes

**Goal: Add musically interesting modes**

### 4.1 Acid Sequencer (Mode 2) (Priority: MEDIUM)

- [ ] Create `modes/02_acid.lua`
  - S1: Octave of active note
  - S2: Note length
  - S3: CC Portamento
  - S4: CC Filter
  - Implement slide/accent logic
  - Generate TB-303 style sequences

**Validation:** Can create acid basslines, sliders control musical parameters.

### 4.2 Additional Modes (Priority: LOW)

Design and implement as desired:

- [ ] Euclidean rhythm generator
- [ ] Arpeggiator
- [ ] Chord strummer
- [ ] Probability-based sequencer
- [ ] Polyrhythm generator
- [ ] Sample slicer
- [ ] Step-modulation sequencer

Use `/design-mode` to brainstorm before implementing.

### 4.3 Multi-Mode Testing (Priority: MEDIUM)

- [ ] Test multiple modes simultaneously
  - Run drum machine on channel 1
  - Run acid sequencer on channel 2
  - Verify MIDI channels are separate
  - Check for timing conflicts

**Validation:** Multiple modes play together without interference.

---

## Phase 5: Teensy Port

**Goal: Deploy to actual hardware**

### 5.1 Teensy Hardware Layer (Priority: HIGH)

- [ ] Create `src/teensy/teensy_hardware.h/cpp`
  - Implement HardwareInterface
  - readButton(): use digitalRead() with debouncing
  - readPot(): use analogRead() with hysteresis
  - sendMidi(): use USB MIDI or Serial MIDI
  - setLED(): use digitalWrite()

- [ ] Create `src/teensy/main.cpp`
  - Arduino setup(): initialize hardware, load song, start engine
  - Arduino loop(): call engine.update()

**Validation:** Code compiles for Teensy (ARM toolchain).

### 5.2 Teensy Build System (Priority: MEDIUM)

- [ ] Create `platformio.ini` (if using PlatformIO)
  - Set platform = teensy
  - Set board = teensy41
  - Set framework = arduino
  - Configure Lua and dependencies

- [ ] OR configure Arduino IDE
  - Add Teensy board support
  - Set up library dependencies
  - Configure serial/USB MIDI

**Validation:** Project builds for Teensy, binary size is acceptable.

### 5.3 Hardware Testing (Priority: HIGH)

- [ ] Flash to Teensy
  - Upload firmware
  - Connect buttons, pots, MIDI out
  - Power on, verify boot

- [ ] Test basic functions
  - Press buttons → LEDs respond
  - Turn pots → values change
  - Program a pattern → MIDI plays

- [ ] Test modes
  - Boot mode: load/save/erase
  - Drum mode: sequence beats
  - Acid mode: play bassline

**Validation:** All features work on physical hardware.

### 5.4 Optimization (Priority: MEDIUM)

- [ ] Profile memory usage
  - Run `/memory-analysis`
  - Verify RAM usage < 1MB
  - Check for stack overflows

- [ ] Profile CPU usage
  - Ensure update loop runs at tempo
  - Check for Lua GC pauses
  - Optimize hot paths

- [ ] Test edge cases
  - Maximum tempo (1000 BPM)
  - All 15 modes playing
  - Rapid button presses

**Validation:** Stable, no crashes, timing is accurate.

### 5.5 LED Feedback (Priority: LOW)

- [ ] Add LED indicators
  - Blink on beat
  - Show current mode (multi-color LED)
  - Flash on save/load
  - Visual feedback for all actions

**Validation:** LEDs provide useful visual feedback during performance.

---

## Phase 6: Polish & Documentation (Optional)

### 6.1 Documentation

- [ ] Write `docs/modes.md` - describe all modes
- [ ] Write `docs/hardware.md` - wiring diagrams, parts list
- [ ] Write `docs/lua_api.md` - Lua API reference
- [ ] Write `docs/midi.md` - MIDI implementation chart
- [ ] Create tutorial videos or guides

### 6.2 Example Content

- [ ] Create example songs in `examples/songs/`
- [ ] Document interesting patterns
- [ ] Share mode ideas

### 6.3 Advanced Features

- [ ] MIDI sync (clock in/out)
- [ ] USB host for MIDI controllers
- [ ] Probability per step
- [ ] Ratcheting/sub-steps
- [ ] Song mode (chain patterns)
- [ ] Performance macros

---

## Current Status

**Phase:** 0 (Planning) - ✅ Complete
**Next Phase:** 1 (Core Foundation)
**Next Task:** Create `src/core/event.h` - Define Event struct

## How to Use This Roadmap

1. **Start with Phase 1, Task 1.1**
2. **Complete tasks in order** (some can be parallelized)
3. **Check off tasks as you complete them** (edit this file)
4. **Test thoroughly** after each task
5. **Move to next phase** only when current phase works

## Getting Help

- Run `/architecture-check` to see current progress
- Read `CLAUDE.md` for architectural details
- Ask questions when design is unclear
- Test incrementally to catch issues early

---

**Remember:** Desktop first, then Teensy. Build incrementally. Test often. Have fun making music! 🎵
