# GRUVBOK Lua API Documentation

## Overview

GRUVBOK modes are implemented in Lua. Each mode is a script that transforms Event data into MIDI messages. The C++ engine calls Lua functions during playback, and Lua responds with MIDI commands.

## Architecture: Dataflow

```
User Input → Data Structure → Playback → Lua → MIDI Output
   ↓              ↓              ↓         ↓        ↓
Buttons        Events         Engine    Mode    Hardware
Sliders        (Song)         (step)   Script   (MIDI)
```

1. **Input**: User presses buttons (B1-B16) and moves sliders (S1-S4)
2. **Storage**: Values are parameter-locked into Events (bit-packed, 29 bits)
3. **Playback**: Engine steps through Events at tempo-sync'd rate
4. **Transform**: Lua `process_event()` receives Event data, returns MIDI
5. **Output**: MIDI scheduler sends note on/off, CC messages with delta timing

## Core Concept: Parameter Locking

When you press a button, the **current slider values** are saved to that Event. Moving sliders later doesn't change existing Events - only affects new Events you create. This is per-step parameter locking.

Example:
```
1. Set S1=60 (pitch), press button 1 → Step 1 locked to pitch 60
2. Set S1=67 (pitch), press button 5 → Step 5 locked to pitch 67
3. Move S1 to any value → Steps 1 and 5 keep their locked values!
```

## Mode Structure

Every mode must implement two functions:

### `init(context)`

Called once when the mode loads.

**Parameters:**
- `context.tempo` (number): Current tempo in BPM
- `context.mode_number` (number): This mode's number (0-14)
- `context.midi_channel` (number): MIDI channel for this mode (same as mode number)

**Example:**
```lua
function init(context)
    print("My mode initialized on channel " .. context.midi_channel)
    -- Initialize any mode-specific state here
end
```

### `process_event(track, event)`

Called for every Event during playback (16 times per bar, per track).

**Parameters:**
- `track` (number): Track number (0-7)
- `event` (table):
  - `event.switch` (boolean): Is this event active?
  - `event.pots` (array): Four parameter values [1-4], each 0-127
    - `event.pots[1]` = S1 value
    - `event.pots[2]` = S2 value
    - `event.pots[3]` = S3 value
    - `event.pots[4]` = S4 value

**Returns:**
- Empty table `{}` (return value is ignored; MIDI events go to internal buffer)

**Example:**
```lua
function process_event(track, event)
    if not event.switch then
        return {}  -- Event is off, do nothing
    end

    local pitch = event.pots[1]    -- S1
    local velocity = event.pots[2]  -- S2

    -- Send MIDI note
    note(pitch, velocity, 0)    -- Note on immediately
    off(pitch, 100)             -- Note off after 100ms

    return {}
end
```

## MIDI API Functions

These C++ functions are exposed to Lua. They directly add events to an internal buffer - **do not try to collect return values**!

### `note(pitch, velocity, [delta])`

Send MIDI note on.

**Parameters:**
- `pitch` (number): MIDI note number (0-127), where 60=C4
- `velocity` (number): Note velocity (1-127)
- `delta` (number, optional): Delay in milliseconds (default: 0)

**Example:**
```lua
note(60, 100, 0)    -- C4 at velocity 100, immediately
note(64, 80, 50)    -- E4 at velocity 80, 50ms from now
```

### `off(pitch, [delta])`

Send MIDI note off.

**Parameters:**
- `pitch` (number): MIDI note number (0-127)
- `delta` (number, optional): Delay in milliseconds (default: 0)

**Example:**
```lua
off(60, 100)  -- Turn off C4 after 100ms
```

**Common Pattern:**
```lua
note(pitch, velocity, 0)   -- Note on now
off(pitch, length)         -- Note off after 'length' ms
```

### `cc(controller, value, [delta])`

Send MIDI Control Change message.

**Parameters:**
- `controller` (number): CC number (0-127)
- `value` (number): CC value (0-127)
- `delta` (number, optional): Delay in milliseconds (default: 0)

**Common CCs:**
- `1`: Modulation wheel
- `7`: Volume
- `10`: Pan
- `11`: Expression
- `64`: Sustain pedal
- `71`: Resonance/Timbre
- `74`: Brightness/Filter cutoff

**Example:**
```lua
cc(74, event.pots[4], 0)   -- Filter cutoff from S4
cc(1, 64, 0)                -- Modulation wheel to middle
```

### `stopall([delta])`

Send "All Notes Off" message (CC 123).

**Parameters:**
- `delta` (number, optional): Delay in milliseconds (default: 0)

**Example:**
```lua
stopall(0)  -- Panic! Stop all notes immediately
```

## Event Data Structure (C++)

For reference, here's how Events are stored in C++:

```cpp
// 29 bits packed into uint32_t:
// Bit 0:     Switch (1 bit)
// Bits 1-7:  Pot 0 (7 bits, 0-127)
// Bits 8-14: Pot 1 (7 bits, 0-127)
// Bits 15-21: Pot 2 (7 bits, 0-127)
// Bits 22-28: Pot 3 (7 bits, 0-127)
```

This bit-packing saves memory (important for Teensy) while providing full MIDI resolution.

## Complete Mode Example: Drum Machine

```lua
--[[
  Mode 1: Drum Machine
  8 tracks, each with different drum sound
]]--

-- Drum note mapping (GM MIDI standard)
local drum_map = {
    36,  -- Track 0: Kick
    38,  -- Track 1: Snare
    42,  -- Track 2: Closed Hi-Hat
    46,  -- Track 3: Open Hi-Hat
    41,  -- Track 4: Low Tom
    43,  -- Track 5: Mid Tom
    45,  -- Track 6: High Tom
    49   -- Track 7: Crash Cymbal
}

function init(context)
    print("Drum machine on channel " .. context.midi_channel)
end

function process_event(track, event)
    if not event.switch then
        return {}
    end

    local drum_note = drum_map[track + 1]  -- Lua is 1-indexed
    local velocity = event.pots[1]          -- S1: Velocity
    local note_length = math.max(10, event.pots[2])  -- S2: Length (min 10ms)

    note(drum_note, velocity, 0)
    off(drum_note, note_length)

    return {}
end
```

## Mode Development Tips

### 1. Use Helper Functions

```lua
-- Map a value from one range to another
function map_range(value, in_min, in_max, out_min, out_max)
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min
end

-- Clamp to range
function clamp(value, min_val, max_val)
    return math.max(min_val, math.min(max_val, value))
end
```

### 2. Use Scales for Melodic Modes

```lua
local minor_scale = {0, 2, 3, 5, 7, 8, 10}  -- Natural minor intervals

function quantize_to_scale(note, scale)
    local octave = math.floor(note / 12)
    local pitch_class = note % 12

    -- Find closest scale degree
    local closest = scale[1]
    local min_dist = math.abs(pitch_class - closest)

    for _, interval in ipairs(scale) do
        local dist = math.abs(pitch_class - interval)
        if dist < min_dist then
            min_dist = dist
            closest = interval
        end
    end

    return octave * 12 + closest
end
```

### 3. Track-Specific Behavior

```lua
function process_event(track, event)
    if track == 0 then
        -- Track 0: Kick drum logic
    elseif track == 7 then
        -- Track 7: Accent/variation
    else
        -- Other tracks
    end
end
```

### 4. Conditional MIDI

```lua
-- Only send filter CC if value is high enough
if event.pots[4] > 20 then
    cc(74, event.pots[4], 0)
end

-- Velocity-sensitive note length
local length = event.pots[2]
if event.pots[1] > 100 then
    length = length * 1.5  -- Longer notes for loud hits
end
```

## Mode Ideas & Patterns

### Arpeggiator
Map S1 to chord root, S2 to arp pattern (up/down/random), S3 to velocity, S4 to note length. Use track number to determine arp step.

### Euclidean Rhythm Generator
Use S1 for number of hits, S2 for total steps, generate Euclidean pattern algorithmically.

### Random/Generative
Use pot values as probability/range parameters, `math.random()` for generative sequences.

### Scale Quantizer
S1 = root note, S2 = scale type (stored as table of scales), quantize all notes to scale.

### Chord Player
Store chord definitions (see modes/03_chords.lua), trigger multiple notes per event.

### Pattern Modifier
Read events from a different track, transform them (transpose, reverse, etc.).

## Debugging

Print to console for debugging:
```lua
print("Track " .. track .. " pitch: " .. event.pots[1])
```

Check if mode is loaded:
```lua
function init(context)
    print("Mode " .. context.mode_number .. " LOADED OK")
end
```

## Performance Notes

- Lua code runs in real-time during playback
- Keep `process_event()` fast - avoid heavy computation
- Pre-calculate tables in `init()` when possible
- Event buffer is pre-allocated, no garbage collection pressure

## MIDI Timing

- All timing is relative (delta time in milliseconds)
- Delta is from the current step's time, not absolute
- MIDI clock (24 PPQN) is sent automatically by engine
- Step interval = (60000 / BPM) / 4 (assuming 16th notes)

## Next Steps

1. Copy `modes/TEMPLATE.lua` to create a new mode
2. Edit slider labels in `src/desktop/gui_main.cpp` function `GetSliderLabel()`
3. Test on desktop with GUI simulator
4. Port to Teensy when ready

For more examples, see the modes directory:
- `modes/01_drums.lua` - Drum machine
- `modes/02_acid.lua` - TB-303 style bassline
- `modes/03_chords.lua` - Polyphonic chord sequencer
- `modes/TEMPLATE.lua` - Documented template
