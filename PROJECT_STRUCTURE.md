# GRUVBOK Project Structure

This document outlines the intended directory structure for GRUVBOK. Not all directories exist yet - they will be created as development progresses.

## Directory Layout

```
gruvbok/
├── .claude/                    # Claude development configuration
│   └── commands/               # Slash commands for common tasks
│       ├── add-mode.md
│       ├── architecture-check.md
│       ├── design-mode.md
│       ├── explain-mode.md
│       └── memory-analysis.md
│
├── src/                        # Source code
│   ├── core/                   # Platform-agnostic core (shared by desktop + Teensy)
│   │   ├── song.h              # Song data structure
│   │   ├── song.cpp
│   │   ├── pattern.h           # Pattern/Track containers
│   │   ├── pattern.cpp
│   │   ├── event.h             # Event with bit-packing
│   │   ├── event.cpp
│   │   ├── engine.h            # Main playback engine
│   │   └── engine.cpp
│   │
│   ├── hardware/               # Hardware abstraction layer
│   │   ├── hardware_interface.h        # Abstract interface
│   │   ├── config.h                    # .ini file parser
│   │   ├── config.cpp
│   │   ├── midi_scheduler.h            # Delta timing and MIDI output
│   │   └── midi_scheduler.cpp
│   │
│   ├── lua_bridge/             # Lua integration
│   │   ├── lua_context.h               # Lua state management
│   │   ├── lua_context.cpp
│   │   ├── lua_api.h                   # C++ functions exposed to Lua
│   │   ├── lua_api.cpp
│   │   ├── mode_loader.h               # Loads .lua mode files
│   │   └── mode_loader.cpp
│   │
│   ├── desktop/                # Desktop-specific implementation
│   │   ├── main.cpp                    # Desktop entry point
│   │   ├── desktop_hardware.h          # Keyboard/mouse emulation
│   │   ├── desktop_hardware.cpp
│   │   └── ui/                         # Optional visual feedback
│   │       └── main_window.cpp
│   │
│   └── teensy/                 # Teensy-specific implementation
│       ├── main.cpp                    # Arduino setup()/loop()
│       ├── teensy_hardware.h           # GPIO, ADC, MIDI
│       └── teensy_hardware.cpp
│
├── modes/                      # Lua mode scripts
│   ├── 00_boot.lua             # Boot mode (load/save/erase)
│   ├── 01_drums.lua            # 808-style drum machine
│   ├── 02_acid.lua             # TB-303 style acid sequencer
│   ├── 03_example.lua          # Template/example mode
│   └── ...                     # Additional modes (euclidean, arp, etc.)
│
├── config/                     # Hardware configuration files
│   ├── default.ini             # Default hardware mapping
│   ├── teensy41.ini            # Teensy 4.1 specific config
│   └── custom.ini              # User custom mappings
│
├── tests/                      # Unit and integration tests
│   ├── test_event.cpp          # Event bit-packing tests
│   ├── test_song.cpp           # Data structure tests
│   ├── test_lua.cpp            # Lua integration tests
│   └── test_scheduler.cpp      # MIDI timing tests
│
├── docs/                       # Additional documentation
│   ├── modes.md                # Mode descriptions
│   ├── hardware.md             # Hardware wiring diagrams
│   ├── midi.md                 # MIDI implementation details
│   └── lua_api.md              # Lua API reference
│
├── examples/                   # Example songs, patterns
│   └── songs/                  # Example .song files
│
├── build/                      # Build output (gitignored)
├── bin/                        # Compiled binaries (gitignored)
│
├── CMakeLists.txt              # Main CMake build file
├── platformio.ini              # Teensy build config (if using PlatformIO)
│
├── .gitignore
├── README.md                   # Project overview
├── CLAUDE.md                   # Claude development guide
├── PROJECT_STRUCTURE.md        # This file
└── LICENSE

```

## File Responsibilities

### Core Layer (`src/core/`)
**Platform-agnostic code that runs identically on desktop and Teensy.**

- **event.h/cpp**: Defines the Event struct with bit-packing for memory efficiency
- **pattern.h/cpp**: Track (16 Events) and Pattern (8 Tracks) containers
- **song.h/cpp**: Song container (15 Modes × 32 Patterns)
- **engine.h/cpp**: Main playback loop, processes Events, calls Lua, schedules MIDI

### Hardware Layer (`src/hardware/`)
**Abstractions that hide platform differences.**

- **hardware_interface.h**: Pure virtual interface for buttons, pots, MIDI, LED
- **config.h/cpp**: Parses .ini files to map hardware (buttons to pins, etc.)
- **midi_scheduler.h/cpp**: Schedules MIDI events with delta timing, manages priority queue

### Lua Bridge (`src/lua_bridge/`)
**Integration between C++ engine and Lua modes.**

- **lua_context.h/cpp**: Manages Lua state, one per mode
- **lua_api.h/cpp**: C++ functions exposed to Lua (note, off, cc, stopall)
- **mode_loader.h/cpp**: Loads .lua files, validates required functions

### Desktop Implementation (`src/desktop/`)
**Desktop-specific code for development and testing.**

- **main.cpp**: Entry point, initializes JUCE/SDL, creates engine
- **desktop_hardware.cpp**: Implements HardwareInterface using keyboard and RtMidi
- **ui/**: Optional visual feedback (button states, current track, etc.)

### Teensy Implementation (`src/teensy/`)
**Teensy-specific code for production hardware.**

- **main.cpp**: Arduino setup() and loop() functions
- **teensy_hardware.cpp**: Implements HardwareInterface using Teensy GPIO/ADC/MIDI

## Build Targets

### Desktop Build
```bash
mkdir build && cd build
cmake .. -DTARGET=desktop
make
./gruvbok-desktop
```

### Teensy Build
```bash
# Using PlatformIO
pio run -t upload

# Or using Teensy Loader / Arduino IDE
# Compile and upload via GUI
```

## Development Flow

1. **Start**: Create core data structures (`src/core/`)
2. **Test**: Build desktop version, test with keyboard
3. **Extend**: Add Lua integration, create modes
4. **Iterate**: Test modes on desktop
5. **Port**: Compile for Teensy, test on hardware
6. **Deploy**: Flash to Teensy, connect MIDI devices

## Quick Start

### For New Developers

1. Read `README.md` - understand the concept
2. Read `CLAUDE.md` - understand the architecture
3. Run `/architecture-check` - see current status
4. Start with Phase 1 tasks (core data structures)

### Common Tasks

- **Add a mode**: `/add-mode` or manually create in `modes/`
- **Check architecture**: `/architecture-check`
- **Design new mode**: `/design-mode`
- **Explain existing mode**: `/explain-mode`
- **Memory analysis**: `/memory-analysis`

## What Exists Now

Currently the project has:
- ✅ README.md (project concept)
- ✅ CLAUDE.md (development guide)
- ✅ PROJECT_STRUCTURE.md (this file)
- ✅ .gitignore
- ✅ .claude/commands/ (slash commands)

Still needed:
- ⏳ All source code (`src/`)
- ⏳ Lua modes (`modes/`)
- ⏳ Build system (CMakeLists.txt)
- ⏳ Tests
- ⏳ Config files

Development is in **Phase 0: Planning** - ready to start implementation!

## Notes

- Desktop version is the **primary development target**
- Teensy version shares the same core code
- All Lua modes work on both platforms
- Use hardware abstraction layer to keep code portable
- Build incrementally: data model → engine → Lua → desktop → Teensy
