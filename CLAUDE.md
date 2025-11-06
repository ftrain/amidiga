# GRUVBOK - Claude Development Guide

## Project Identity

GRUVBOK is a hardware MIDI groovebox that operates on a unique "always playing" philosophy. The system continuously loops through a data structure that users build by pressing buttons and moving knobs. This is NOT a traditional DAW or sequencer - it's a **musical instrument** with immediate tactile feedback.

**Core Philosophy:**
- The "song" is always playing and looping
- Users program events in real-time by hitting buttons B1-B16 and moving sliders S1-S4
- Changes are instantly persisted and audible
- The system stops playing only when it runs out of programmed data

## Architecture Overview

GRUVBOK uses a two-layer architecture:

### Layer 1: C++ "OS" Layer
- Manages the core data structure (Song → Mode → Pattern → Track → Event)
- Handles hardware I/O (buttons, pots, MIDI output)
- Manages state (current mode/pattern/track selection)
- Handles persistence (load/save)
- Schedules MIDI events with delta timing
- Provides clean API to Lua

### Layer 2: Lua Mode Layer
- Each mode (0-15) is a separate Lua script
- Transforms Event data into MIDI messages
- Each mode plays on a different MIDI channel
- Lua receives: Track number, Event (Switch + 4 Pots)
- Lua returns: MIDI events with delta timing for note-off

**Key Insight:** The C++ layer is the "what" (data structure), Lua is the "how" (MIDI interpretation).

## Data Model Deep Dive

```
Song = Mode[15]        // 15 simultaneous modes (MIDI channels)
Mode = Pattern[32]     // 32 patterns per mode
Pattern = Track[8]     // 8 tracks per pattern
Track = Event[16]      // 16 events per track (matches 16 buttons!)
Event = {Switch, Pot[4]}  // Switch on/off + 4 slider values

Where:
- Switch = 0|1 (boolean)
- Pot = 0..127 (MIDI value)
```

**Why This Structure:**
- 16 events match the 16 momentary buttons on the hardware
- 4 pots match the 4 slider pots
- Each Event captures a "snapshot" of what the user programmed
- The system plays through Events in sequence, looping forever
- Multiple modes play simultaneously on different MIDI channels

**Memory Considerations:**
- Event could be bit-packed: 1 bit for switch + 4×7 bits for pots = 29 bits (fits in uint32_t)
- Consider memory layout for Teensy 4.1 (1MB RAM, but be efficient)
- No dynamic allocation in playback loop

## Development Workflow: Desktop First

**Critical Decision:** Build desktop version FIRST, then deploy to Teensy.

### Why Desktop First?
1. Faster iteration (compile times)
2. Easier debugging (printf, debuggers, memory tools)
3. Can test without physical hardware
4. Same C++ and Lua code runs on both platforms

### Recommended Tech Stack

**Desktop:**
- **JUCE** for audio/MIDI and potentially GUI, OR
- **SDL** for input emulation + RtMidi for MIDI output
- **Lua 5.4** (lightweight, embeddable)
- **CMake** for cross-platform builds

**Teensy:**
- **Teensy 4.1** (600 MHz ARM Cortex-M7, 1MB RAM)
- **Arduino framework** or **Teensy cores**
- Same Lua 5.4
- Same core C++ code (hardware abstraction layer)

## Code Organization

Proposed directory structure:

```
gruvbok/
├── src/
│   ├── core/              # Platform-agnostic core
│   │   ├── song.h/cpp     # Song data structure
│   │   ├── pattern.h/cpp  # Pattern/Track/Event
│   │   ├── event.h/cpp    # Event bit-packing
│   │   └── engine.h/cpp   # Playback engine
│   ├── hardware/          # Hardware abstraction
│   │   ├── hardware_interface.h  # Abstract interface
│   │   ├── config.h/cpp   # .ini parser
│   │   └── midi_scheduler.h/cpp  # MIDI timing
│   ├── lua_bridge/        # Lua integration
│   │   ├── lua_context.h/cpp
│   │   ├── lua_api.h/cpp  # note(), off(), stopall()
│   │   └── mode_loader.h/cpp
│   ├── desktop/           # Desktop-specific
│   │   ├── main.cpp
│   │   ├── desktop_hardware.h/cpp  # Emulates buttons/pots
│   │   └── ui/ (optional) # Visual feedback
│   └── teensy/            # Teensy-specific
│       ├── main.cpp
│       └── teensy_hardware.h/cpp   # Real GPIO/ADC
├── modes/                 # Lua mode scripts
│   ├── 00_boot.lua
│   ├── 01_drums.lua
│   ├── 02_acid.lua
│   └── ...
├── config/                # Hardware configurations
│   ├── default.ini
│   └── custom.ini
├── tests/
├── docs/
├── CMakeLists.txt
└── README.md
```

## Lua Integration

### Lua Mode Contract

Each Lua mode must define:

```lua
-- Called once when mode loads
function init(context)
  -- context provides: tempo, mode_number, midi_channel
  -- Initialize any mode-specific state
end

-- Called for each Event during playback
-- track: 0-7
-- event: {switch = true/false, pots = {s1, s2, s3, s4}}
-- Returns: array of MIDI events
function process_event(track, event)
  local midi_events = {}

  if event.switch then
    -- Use convenience API
    table.insert(midi_events, note(60, 127))      -- note on
    table.insert(midi_events, off(60, 100))       -- note off at +100ms
  end

  -- Can also send CC messages
  table.insert(midi_events, cc(74, event.pots[4])) -- filter

  return midi_events
end
```

### Lua API Functions (provided by C++)

```lua
note(pitch, velocity, [delta])  -- Note on (default delta=0)
off(pitch, [delta])             -- Note off
cc(controller, value, [delta])  -- Control change
stopall([delta])                -- All notes off
```

**Delta timing:** All deltas are relative to current Event time (in milliseconds).

### Example: Drum Machine Mode

```lua
-- modes/01_drums.lua
local drum_map = {61, 62, 63, 64, 65, 66, 67, 68} -- MIDI notes

function init(context)
  -- Setup if needed
end

function process_event(track, event)
  local midi = {}

  if event.switch then
    local velocity = event.pots[1]  -- Velocity from S1
    local note_num = drum_map[track + 1]

    table.insert(midi, note(note_num, velocity))
    table.insert(midi, off(note_num, 10))  -- 10ms note
  end

  -- Track 8 is accent
  if track == 7 and event.switch then
    -- Boost velocity for all notes
  end

  return midi
end
```

## Hardware Abstraction Layer

To share code between desktop and Teensy:

```cpp
// hardware_interface.h
class HardwareInterface {
public:
    virtual void init() = 0;
    virtual bool readButton(int button) = 0;  // B1-B16
    virtual int readPot(int pot) = 0;         // R1-R4, S1-S4
    virtual void sendMidiMessage(uint8_t* data, size_t len) = 0;
    virtual void setLED(bool on) = 0;
    virtual uint32_t getMillis() = 0;
};

// Desktop: Keyboard emulates buttons, sliders in UI
class DesktopHardware : public HardwareInterface { ... };

// Teensy: GPIO and ADC
class TeensyHardware : public HardwareInterface { ... };
```

## Configuration System

`.ini` file format for hardware mapping:

```ini
# hardware.ini
[buttons]
B1=2   # Pin 2
B2=3   # Pin 3
# ... etc

[rotary_pots]
R1=A0  # Analog pin A0 - Mode
R2=A1  # Analog pin A1 - Tempo
R3=A2  # Analog pin A2 - Pattern
R4=A3  # Analog pin A3 - Track

[slider_pots]
S1=A4
S2=A5
S3=A6
S4=A7

[midi]
channel_offset=0  # First mode uses channel 0
```

Comments allowed with `#`. Parser should be simple and robust.

## Performance Constraints

### Real-Time Requirements

1. **No allocations in audio/MIDI thread**
   - Pre-allocate all data structures
   - Use fixed-size arrays or stack allocation
   - Lua should use pre-allocated states

2. **MIDI timing precision**
   - Events must be scheduled with <1ms jitter
   - Use high-resolution timer (not millis())
   - Priority queue for scheduled events

3. **Teensy memory limits**
   - 1MB RAM on Teensy 4.1
   - Calculate worst-case: 15 modes × 32 patterns × 8 tracks × 16 events × 4 bytes ≈ 245KB
   - Leaves plenty for code and stack

### Critical Path

During playback loop:
1. Check time → trigger Events that are due
2. Call Lua function (should be fast, <1ms)
3. Schedule returned MIDI events
4. Send MIDI messages at scheduled time
5. Handle button/pot input (non-blocking)
6. Update data structure if user made changes

**No blocking operations allowed in playback thread.**

## Implementation Priorities

### Phase 1: Core Foundation (Desktop)
1. Define Event, Track, Pattern, Mode, Song classes
2. Implement bit-packing for Event (optimize memory)
3. Create Engine that loops through data structure
4. Build hardware abstraction interface
5. Create desktop hardware implementation (keyboard input)
6. Basic MIDI output (RtMidi or JUCE MIDI)

### Phase 2: Lua Integration
1. Embed Lua interpreter
2. Create Lua API (note, off, cc, stopall)
3. Implement mode loader (loads .lua files)
4. Create boot mode (Mode 0) for load/save
5. Test with simple drum machine mode

### Phase 3: Full Desktop Features
1. Implement .ini config parser
2. Add global pot handlers (R1-R4)
3. Implement pattern/track switching
4. Add persistence (save/load songs)
5. Visual feedback (optional UI)

### Phase 4: More Modes
1. Drum machine (Mode 1)
2. Acid sequencer (Mode 2)
3. Test multiple modes playing simultaneously
4. Verify MIDI channel separation

### Phase 5: Teensy Port
1. Implement teensy_hardware.cpp (GPIO, ADC)
2. Port main loop to Arduino framework
3. Test on physical hardware
4. Optimize for size and speed
5. Flash LED feedback

## Common Development Tasks

### Adding a New Mode

1. Create `modes/XX_modename.lua`
2. Implement `init()` and `process_event()` functions
3. Test on desktop with keyboard input
4. Map S1-S4 sliders to musically useful parameters
5. Document mode in README

### Testing Without Hardware

Desktop version should:
- Map computer keyboard keys to B1-B16
- Use trackpad/mouse for pot emulation
- Show visual feedback of current state
- Print MIDI messages to console (debug mode)
- Send actual MIDI to virtual MIDI port

### Debugging MIDI Issues

1. Enable verbose logging in desktop mode
2. Print Event contents before Lua call
3. Print MIDI events returned from Lua
4. Use MIDI monitor tool (MIDI-OX, VMPK)
5. Check delta timing calculations

### Memory Analysis for Teensy

```bash
# After building for Teensy
arm-none-eabi-size --format=SysV gruvbok.elf
# Check RAM usage is well under 1MB
```

## Technical Decisions & Rationale

### Why Lua?
- Lightweight, embeds easily
- Fast enough for real-time use
- Familiar scripting syntax
- Easy to hot-reload modes during development

### Why Desktop First?
- 10x faster development cycle
- Can test logic without hardware
- Same code runs on both (confidence in port)
- Easier to collaborate (others don't need Teensy)

### Why Always Playing?
- Immediate feedback (musician's workflow)
- Simpler state machine (no play/stop button)
- Natural for live performance
- Forces efficient, real-time design

### Why Bit-Packed Events?
- Memory efficiency for Teensy
- Cache-friendly (more Events per cache line)
- Fixed size helps real-time constraints

## Key Gotchas

1. **Lua GC pauses:** Configure Lua GC for predictable, small pauses
2. **MIDI buffer overruns:** Limit max events per Lua call (e.g., 16)
3. **Button debouncing:** Hardware buttons need debounce logic
4. **Pot jitter:** Apply hysteresis to analog pot readings
5. **Tempo changes:** Recalculate all event timing on tempo change
6. **Mode switching:** Safely stop all notes before switching Lua context

## Testing Strategy

### Unit Tests
- Event bit-packing/unpacking
- Song data structure operations
- MIDI scheduling queue
- Config file parser

### Integration Tests
- Lua mode loading and execution
- Full playback loop (mocked hardware)
- Multiple modes simultaneously
- Save/load persistence

### Hardware Tests (Teensy)
- Button responsiveness
- Pot value accuracy
- MIDI timing jitter
- LED feedback

## Resources & References

- **Teensy 4.1:** https://www.pjrc.com/store/teensy41.html
- **JUCE Framework:** https://juce.com/
- **SDL:** https://www.libsdl.org/
- **RtMidi:** https://www.music.mcgill.ca/~gary/rtmidi/
- **Lua 5.4:** https://www.lua.org/manual/5.4/
- **MIDI Specification:** https://www.midi.org/specifications

## Questions to Consider

- Should desktop version have a GUI or be terminal-based?
- What's the best event scheduling data structure? (Priority queue, circular buffer?)
- Should Lua states be per-mode or single shared state?
- How to handle tempo changes smoothly? (Stretch events vs. snap to grid?)
- What file format for song persistence? (Binary, JSON, custom?)
- Should .ini support hot-reload on desktop?

## Getting Started

1. Read README.md for project overview
2. Read this CLAUDE.md for technical details
3. Start with Phase 1: Core Foundation
4. Build incrementally, test frequently
5. Desktop first, Teensy later
6. Ask questions when architecture is unclear

---

**Remember:** GRUVBOK is a musical instrument. Every technical decision should prioritize immediate feedback, reliability, and the joy of making music by pressing buttons and twisting knobs.
