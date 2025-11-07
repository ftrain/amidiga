# GRUVBOK

**A hardware MIDI groovebox with Lua-scriptable modes - Desktop first, Teensy deployment ready**

## Status: Desktop Complete ✅ | Teensy Ready for Testing 🚧

**What Works:**
- ✅ Desktop GUI simulator with full feature set
- ✅ 15 modes (0-14): Song sequencer, drums, acid, chords, + 11 experimental modes
- ✅ Save/Load songs (JSON format)
- ✅ Multi-timbral playback (15 MIDI channels)
- ✅ Full test suite (56/56 tests passing)
- ✅ Teensy 4.1 firmware (Lua + SD card support, awaiting hardware testing)

## Quick Start

```bash
# Install dependencies (macOS/Linux)
brew install cmake lua sdl2  # macOS
apt install cmake lua5.4 libsdl2-dev  # Linux

# Build and run desktop simulator
cmake -B build && cmake --build build
bin/gruvbok  # GUI version
```

## Philosophy: Always Playing

GRUVBOK is different from traditional sequencers - **it's always playing**. You build a data structure by pressing buttons and moving sliders, and the system continuously loops through it, transforming Events into MIDI via Lua scripts.

## Architecture

```
Hardware → Data Structure → Playback Engine → Lua Modes → MIDI Output
   ↓            ↓               ↓               ↓            ↓
Buttons      Events          Tempo-sync      Transform    Hardware
Pots      (parameter-locked)  Stepping        to MIDI      (USB)
```

### Data Model

```
Song = Mode[15]        # 15 simultaneous modes (MIDI channels)
Mode = Pattern[32]     # 32 patterns per mode
Pattern = Track[8]     # 8 tracks per pattern
Track = Event[16]      # 16 events (matches 16 buttons!)
Event = {Switch, Pot[4]}  # 1 switch + 4 sliders (29 bits packed)
```

**Memory footprint:** 245KB for all event data (61,440 events)

### Controls

**Rotary Pots (R1-R4):**
- R1: Mode (0-14)
- R2: Tempo (60-240 BPM)
- R3: Pattern (0-31)
- R4: Track (0-7)

**Buttons (B1-B16):** Toggle events on/off (16 steps per track)

**Slider Pots (S1-S4):** Mode-specific parameters (velocity, pitch, filter, etc.)

## Implemented Modes

**Mode 0:** Song/Pattern Sequencer (controls which pattern plays across all modes)
**Mode 1:** Drum Machine (8 tracks, GM drum sounds)
**Mode 2:** Acid Sequencer (TB-303 style bassline with slide/filter)
**Mode 3:** Chord Sequencer (16 chord types, polyphonic)
**Modes 4-7:** Reserved for future use
**Mode 8:** Drunk Sequencer (random pitch walks)
**Mode 9:** Cellular Automaton (Conway's Game of Life)
**Mode 10:** Wave Table Scanner (smooth pitch scanning)
**Mode 11:** MIDI Mangler (glitch effects)
**Mode 12:** Lunar Phase (28-step sinusoidal evolution)
**Mode 13:** Markov Chain (probabilistic melody)
**Mode 14:** Tornado Spiral (Shepard tone spirals)

## Hardware

**Target Platform:** Teensy 4.1 (600 MHz ARM, 1MB RAM, microSD slot)

**Required Hardware:**
- 16× momentary buttons (B1-B16)
- 4× rotary pots 10kΩ (R1-R4)
- 4× slide pots 10kΩ (S1-S4)
- 1× LED (tempo indicator)
- USB MIDI output

**See:** `docs/TEENSY_DEPLOYMENT_GUIDE.md` for complete build instructions

## Documentation

- **CLAUDE.md** - Complete technical architecture and development guide
- **QUICKSTART.md** - Get desktop version running in 5 minutes
- **docs/LUA_API.md** - Create custom modes with Lua
- **docs/TEENSY_DEPLOYMENT_GUIDE.md** - Deploy to Teensy hardware
- **DEVELOPMENT_ROADMAP.md** - Implementation status

## Creating Custom Modes

Modes are Lua scripts. Example:

```lua
function init(context)
    -- Called once when mode loads
end

function process_event(track, event)
    if event.switch then
        local pitch = event.pots[1]     -- S1
        local velocity = event.pots[2]  -- S2

        note(pitch, velocity)    -- MIDI note on
        off(pitch, 100)          -- Note off after 100ms
    end
end
```

**See:** `modes/TEMPLATE.lua` and `docs/LUA_API.md`

## Key Features

**Parameter Locking:** Slider values are captured per-step when you press buttons
**Multi-timbral:** All 15 modes play simultaneously on separate MIDI channels
**Desktop-first:** Develop and test without hardware
**Real-time:** Immediate feedback, no play/stop button needed
**Scriptable:** Lua modes for unlimited creative possibilities

## Project Structure

```
amidiga/
├── src/
│   ├── core/          # Platform-agnostic engine
│   ├── hardware/      # Hardware abstraction
│   ├── lua_bridge/    # Lua integration
│   ├── desktop/       # Desktop GUI (SDL2 + ImGui)
│   └── teensy/        # Teensy 4.1 firmware
├── modes/             # Lua mode scripts (00-14)
├── docs/              # Documentation
├── tests/             # Test suite (56 tests, 100% passing)
└── bin/               # Compiled executables
```

## Development

**Built with:** C++17, Lua 5.4, SDL2, RtMidi, Dear ImGui, CMake

**Tested on:** macOS, Linux (Windows compatible)

---

**GRUVBOK is ready to make music - press buttons, twist knobs, and groove! 🎵**







