# GRUVBOK Lua Mode Development Guide

Complete reference for creating musical modes in Lua for GRUVBOK.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Mode Metadata](#mode-metadata)
3. [Context Reference](#context-reference)
4. [API Functions](#api-functions)
5. [Mode 0 Master Control](#mode-0-master-control)
6. [Scale Systems](#scale-systems)
7. [Best Practices](#best-practices)
8. [Complete Examples](#complete-examples)

---

## Quick Start

### Creating a New Mode

1. **Copy the template:**
   ```bash
   cp modes/TEMPLATE.lua modes/05_mymode.lua
   ```

2. **Set mode metadata:**
   ```lua
   MODE_NAME = "My Mode"
   SLIDER_LABELS = {"Pitch", "Length", "Filter", "Resonance"}
   ```

3. **Implement `init()` and `process_event()`:**
   ```lua
   function init(context)
     -- Initialize your mode with context
   end

   function process_event(track, event)
     -- Process each step's event
   end
   ```

4. **Load in desktop app:**
   - Place in `modes/` directory
   - Mode number matches filename (e.g., `05_mymode.lua` = Mode 5)
   - Restart app or hot-reload if supported

---

## Mode Metadata

### Required Metadata

```lua
-- Mode name (shown in GUI)
MODE_NAME = "Acid Bass"
```

### Optional Metadata

```lua
-- Slider labels (provides contextual UI labels for S1-S4)
SLIDER_LABELS = {"Pitch", "Length", "Slide", "Filter"}
-- If not defined, defaults to: {"S1", "S2", "S3", "S4"}
```

**Note:** These must be defined at the top level (not inside functions).

---

## Context Reference

### The `init()` Function

Called once when the mode loads and whenever global parameters change (tempo, Mode 0 settings).

**Signature:**
```lua
function init(context)
  -- Your initialization code
end
```

**Context Table Fields:**

| Field | Type | Range | Description |
|-------|------|-------|-------------|
| `tempo` | int | 1-1000 | Current tempo in BPM |
| `mode_number` | int | 0-14 | This mode's number |
| `midi_channel` | int | 0-14 | MIDI channel (same as mode_number) |
| `scale_root` | int | 0-11 | Global scale root from Mode 0 (0=C, 1=C#, 2=D, ..., 11=B) |
| `scale_type` | int | 0-7 | Global scale type from Mode 0 (see [Scale Systems](#scale-systems)) |
| `velocity_offset` | int | -64 to +63 | Per-mode velocity adjustment from Mode 0 |

**Example:**
```lua
local velocity_offset = 0
local scale = {0, 2, 4, 5, 7, 9, 11}  -- Major scale
local base_note = 60

function init(context)
  print("Mode " .. context.mode_number .. " @ " .. context.tempo .. " BPM")

  -- Store Mode 0 context
  velocity_offset = context.velocity_offset or 0

  -- Build scale from Mode 0
  local scale_types = {
    {0, 2, 4, 5, 7, 9, 11},  -- Ionian
    {0, 2, 3, 5, 7, 9, 10},  -- Dorian
    -- ... etc
  }
  local scale_idx = (context.scale_type or 0) + 1
  scale = scale_types[scale_idx]
  base_note = 60 + (context.scale_root or 0)

  print("  Scale root: " .. context.scale_root .. ", velocity offset: " .. velocity_offset)
end
```

**When is `init()` called?**
- When mode first loads
- After tempo changes (debounced, ~1 second delay)
- When Mode 0 parameters change (scale, velocity)

---

### The `process_event()` Function

Called for every step (16th note) of every track during playback.

**Signature:**
```lua
function process_event(track, event)
  -- Your event processing code
  return {}  -- Return value is ignored (see note below)
end
```

**Parameters:**

| Parameter | Type | Range | Description |
|-----------|------|-------|-------------|
| `track` | int | 0-7 | Current track number |
| `event.switch` | bool | true/false | Button state for this step |
| `event.pots[1]` | int | 0-127 | Slider S1 value (parameter-locked) |
| `event.pots[2]` | int | 0-127 | Slider S2 value (parameter-locked) |
| `event.pots[3]` | int | 0-127 | Slider S3 value (parameter-locked) |
| `event.pots[4]` | int | 0-127 | Slider S4 value (parameter-locked) |

**Important Notes:**
- `event.pots` is **1-indexed** (Lua convention): use `event.pots[1]` through `event.pots[4]`
- Values are **parameter-locked**: captured when button is pressed, not live slider values
- Called even if `event.switch` is false (check it yourself if needed)
- Return value is **ignored** (see API Functions below)

**Example:**
```lua
function process_event(track, event)
  -- Skip if step is off
  if not event.switch then
    return {}
  end

  -- Get parameters
  local pitch = event.pots[1]      -- S1
  local velocity = event.pots[2]   -- S2
  local length = event.pots[3]     -- S3
  local filter = event.pots[4]     -- S4

  -- Generate MIDI
  note(pitch, velocity, 0)
  off(pitch, length)
  cc(74, filter, 0)  -- Filter cutoff

  return {}  -- Required but ignored
end
```

---

## API Functions

All API functions directly add MIDI events to an internal buffer. **Do not collect return values** - just call them and they'll be scheduled automatically.

### `note(pitch, velocity, [delta])`

Send a MIDI Note On message.

**Parameters:**
- `pitch` (int, 0-127): MIDI note number
- `velocity` (int, 1-127): Note velocity (0 is reserved for Note Off)
- `delta` (int, optional, default=0): Delay in milliseconds

**Example:**
```lua
note(60, 100, 0)      -- C4, velocity 100, immediate
note(64, 80, 50)      -- E4, velocity 80, after 50ms
```

---

### `off(pitch, [delta])`

Send a MIDI Note Off message.

**Parameters:**
- `pitch` (int, 0-127): MIDI note number
- `delta` (int, optional, default=0): Delay in milliseconds

**Example:**
```lua
note(60, 100, 0)   -- Note on
off(60, 500)       -- Note off after 500ms
```

**Typical Pattern:**
```lua
local note_length = event.pots[2]  -- S2 controls length
note(pitch, velocity, 0)
off(pitch, note_length)
```

---

### `cc(controller, value, [delta])`

Send a MIDI Continuous Controller (CC) message.

**Parameters:**
- `controller` (int, 0-127): CC number
- `value` (int, 0-127): CC value
- `delta` (int, optional, default=0): Delay in milliseconds

**Common CC Numbers:**
- `1` - Modulation Wheel
- `5` - Portamento Time
- `7` - Volume
- `10` - Pan
- `11` - Expression
- `64` - Sustain Pedal
- `65` - Portamento On/Off
- `71` - Resonance/Timbre
- `74` - Brightness/Cutoff
- `91` - Reverb Send
- `93` - Chorus Send

**Example:**
```lua
local filter_cutoff = event.pots[4]
cc(74, filter_cutoff, 0)  -- Send filter CC

-- Portamento (slide)
local slide_amount = event.pots[3]
if slide_amount > 20 then
  cc(65, 127, 0)           -- Portamento on
  cc(5, slide_amount, 0)   -- Portamento time
else
  cc(65, 0, 0)             -- Portamento off
end
```

---

### `stopall([delta])`

Send All Notes Off (CC 123).

**Parameters:**
- `delta` (int, optional, default=0): Delay in milliseconds

**Example:**
```lua
-- Emergency stop
stopall(0)

-- Stop after 2 seconds
stopall(2000)
```

**Use Case:** Pattern change, emergency stop, or mode reset.

---

### `led(pattern_name, [brightness])`

Trigger an LED pattern (desktop and Teensy).

**Parameters:**
- `pattern_name` (string): Pattern name
  - `"tempo"` - Single beat pulse (default)
  - `"held"` - Fast double-blink
  - `"saving"` - Rapid blinks (5 times)
  - `"loading"` - Slow pulse
  - `"error"` - Triple fast blink
  - `"mirror"` - Alternating long/short
- `brightness` (int, optional, 0-255, default=255): LED brightness (PWM on Teensy)

**Example:**
```lua
-- Trigger on certain steps
if track == 0 and event.switch then
  led("held", 200)
end

-- Error condition
if pitch < 0 or pitch > 127 then
  led("error")
end
```

---

## Mode 0 Master Control

Mode 0 is the **master control layer** that coordinates all other modes (1-14).

### How Mode 0 Works

1. **Runs at 1/16th speed:** One Mode 0 step = 16 normal steps (one bar)
2. **Loop length:** Dynamically set by highest button pressed (1-16 bars)
3. **Track mapping:** Tracks 0-13 in Mode 0 configure modes 1-14
4. **No MIDI output:** Mode 0 doesn't play notes, only controls other modes

### Mode 0 Parameters

When editing Mode 0 (R1=0), each step configures **one target mode** (selected by R4):

| Slider | Parameter | Range | Description |
|--------|-----------|-------|-------------|
| S1 | Pattern | 0-31 | Which pattern the target mode plays |
| S2 | Scale Root | 0-11 | Global scale root (C-B) |
| S3 | Scale Type | 0-7 | Global scale type (see below) |
| S4 | Velocity Offset | -64 to +63 | Per-mode velocity adjustment |

### Mode 0 Workflow Example

**Goal:** Make Mode 2 (Acid) play pattern 5 in D Dorian with +20 velocity boost.

1. **Enter Mode 0:** Turn R1 to 0
2. **Select target mode:** Turn R4 to select Mode 2
3. **Set parameters:**
   - S1 = Pattern 5 (slider value ~40)
   - S2 = Root D (slider value ~21)
   - S3 = Dorian (slider value ~16)
   - S4 = Velocity +20 (slider value ~84, since 64 = zero offset)
4. **Program step:** Press B1 to set bar 1
5. **Result:** When Mode 0's bar 1 plays, Mode 2 uses these settings

### Understanding Context in Your Mode

Your Lua mode receives Mode 0 parameters in the `init()` context:

```lua
function init(context)
  -- Mode 0 provides these globally:
  local root = context.scale_root      -- 0-11 (C-B)
  local type = context.scale_type      -- 0-7 (scale type)
  local vel_offset = context.velocity_offset  -- -64 to +63

  -- Use them to configure your mode
  base_note = 36 + root  -- Transpose base note
  -- Build scale from type (see Scale Systems section)
  -- Store velocity offset for use in process_event()
end
```

**When do these update?**
- When Mode 0 step changes (every bar)
- `init()` is called with new context
- Your mode rebuilds its scale/settings

---

## Scale Systems

Mode 0's S3 parameter selects from 8 scale types. Here's how to use them in your mode.

### Scale Type Definitions

```lua
local scale_types = {
  {0, 2, 4, 5, 7, 9, 11},    -- 0: Ionian (Major)
  {0, 2, 3, 5, 7, 9, 10},    -- 1: Dorian
  {0, 1, 3, 5, 7, 8, 10},    -- 2: Phrygian
  {0, 2, 4, 6, 7, 9, 11},    -- 3: Lydian
  {0, 2, 4, 5, 7, 9, 10},    -- 4: Mixolydian
  {0, 2, 3, 5, 7, 8, 10},    -- 5: Aeolian (Natural Minor)
  {0, 1, 3, 5, 6, 8, 10},    -- 6: Locrian
  {0, 3, 5, 7, 10}           -- 7: Minor Pentatonic
}

local scale_names = {
  "Ionian", "Dorian", "Phrygian", "Lydian",
  "Mixolydian", "Aeolian", "Locrian", "Min Pent"
}
```

### Using Scales in Your Mode

**Option 1: Build scale from Mode 0 context (recommended)**

```lua
local scale_types = { /* ... as above ... */ }
local scale = {0, 2, 4, 5, 7, 9, 11}  -- Default
local base_note = 36

function init(context)
  -- Get scale type from Mode 0 (convert to 1-indexed)
  local scale_idx = (context.scale_type or 0) + 1
  scale_idx = math.max(1, math.min(#scale_types, scale_idx))

  -- Build scale
  scale = scale_types[scale_idx]

  -- Transpose by scale root
  base_note = 36 + (context.scale_root or 0)

  print("Using " .. scale_names[scale_idx] .. " root=" .. base_note)
end
```

**Option 2: Use your own scale (ignore Mode 0)**

```lua
-- For modes that don't need scales (drums, noise, etc.)
local velocity_offset = 0

function init(context)
  -- Just store velocity offset, ignore scale
  velocity_offset = context.velocity_offset or 0
end
```

### Mapping Slider Values to Scale Notes

**Example: Map S1 (0-127) to scale notes across 3 octaves**

```lua
function process_event(track, event)
  if not event.switch then return {} end

  local s1_value = event.pots[1]  -- 0-127

  -- Determine scale degree (which note in the scale)
  local scale_index = math.floor((s1_value * #scale) / 128) + 1
  scale_index = math.max(1, math.min(#scale, scale_index))
  local note_offset = scale[scale_index]

  -- Determine octave
  local octave = math.floor(s1_value / 43)  -- 128/3 ≈ 43
  local octave_shift = octave * 12

  -- Final pitch
  local pitch = base_note + note_offset + octave_shift
  pitch = math.max(0, math.min(127, pitch))

  -- Send note
  note(pitch, 100, 0)
  off(pitch, 200)

  return {}
end
```

### Chromatic (Non-Scale) Pitch Control

**Example: Direct MIDI note mapping (ignore Mode 0 scale)**

```lua
function process_event(track, event)
  if not event.switch then return {} end

  -- S1 directly maps to MIDI note (with transpose)
  local pitch = event.pots[1]

  -- Apply Mode 0 transpose (scale_root) only
  pitch = pitch + (context.scale_root or 0)
  pitch = math.max(0, math.min(127, pitch))

  note(pitch, 100, 0)
  off(pitch, 200)

  return {}
end
```

---

## Best Practices

### Performance

**DO:**
- ✅ Pre-calculate scales in `init()`
- ✅ Store Mode 0 context in module-level variables
- ✅ Use simple math in `process_event()`
- ✅ Clamp values to MIDI range (0-127)

**DON'T:**
- ❌ Allocate tables in `process_event()` (use module-level)
- ❌ Call `print()` in `process_event()` (too frequent)
- ❌ Perform heavy computation per-step (use lookup tables)

**Example: Good vs. Bad**

```lua
-- ❌ BAD: Allocates table every step
function process_event(track, event)
  local scale = {0, 2, 4, 5, 7, 9, 11}  -- DON'T DO THIS
  -- ...
end

-- ✅ GOOD: Pre-allocated at module level
local scale = {0, 2, 4, 5, 7, 9, 11}

function init(context)
  -- Modify scale here if needed
end

function process_event(track, event)
  -- Use pre-allocated scale
end
```

### Parameter Mapping

**Velocity:**
```lua
-- S1 controls velocity (0-127)
local velocity = event.pots[1]

-- Apply Mode 0 offset
velocity = velocity + (velocity_offset or 0)

-- Clamp to valid range
velocity = math.max(1, math.min(127, velocity))
```

**Note Length:**
```lua
-- S2 controls note length (10-500ms)
local length = 10 + math.floor((event.pots[2] / 127.0) * 490)
```

**Octave Selection:**
```lua
-- S3 controls octave (-2 to +2)
local octave = math.floor((event.pots[3] / 127.0) * 5) - 2
local pitch = base_pitch + (octave * 12)
```

**Filter Cutoff:**
```lua
-- S4 controls filter (CC 74)
cc(74, event.pots[4], 0)
```

### Multi-Track Usage

**Example: Each track plays different drum sound**

```lua
local drum_notes = {36, 38, 42, 46, 45, 47, 50, 49}

function process_event(track, event)
  if not event.switch then return {} end

  local drum_note = drum_notes[track + 1]  -- track is 0-7
  local velocity = event.pots[1]

  note(drum_note, velocity, 0)
  off(drum_note, 10)

  return {}
end
```

**Example: Track selects chord inversion**

```lua
local chord = {0, 4, 7}  -- Major triad

function process_event(track, event)
  if not event.switch then return {} end

  -- Track determines inversion
  local inversion = track % 3

  for i, interval in ipairs(chord) do
    local pitch = 60 + interval + (inversion * 12)
    note(pitch, 100, 0)
    off(pitch, 500)
  end

  return {}
end
```

### Debugging

**Use `print()` in `init()` only:**
```lua
function init(context)
  print("Mode " .. context.mode_number .. " initialized")
  print("  Tempo: " .. context.tempo .. " BPM")
  print("  Scale root: " .. context.scale_root)
  print("  Velocity offset: " .. context.velocity_offset)
end

-- ❌ DON'T print in process_event() - too frequent!
```

**Desktop MIDI Monitor:**
- Use a MIDI monitor tool (MIDI-OX, VMPK)
- Check that note on/off pairs match
- Verify CC values are in range
- Watch for stuck notes (missing note off)

### Common Pitfalls

**1. Lua array indexing (1-based, not 0-based)**
```lua
-- ❌ WRONG
local value = event.pots[0]  -- nil!

-- ✅ CORRECT
local value = event.pots[1]  -- S1

-- ❌ WRONG
local note = drum_map[track]  -- off by one if track=0

-- ✅ CORRECT
local note = drum_map[track + 1]  -- track 0 → index 1
```

**2. Forgetting to clamp values**
```lua
-- ❌ WRONG: Can exceed MIDI range
local pitch = base_note + offset  -- Could be > 127

-- ✅ CORRECT: Clamp to valid range
local pitch = base_note + offset
pitch = math.max(0, math.min(127, pitch))
```

**3. Missing note off**
```lua
-- ❌ WRONG: Stuck note
note(60, 100, 0)
-- Missing off()!

-- ✅ CORRECT: Always pair note on/off
note(60, 100, 0)
off(60, 500)
```

**4. Ignoring switch state**
```lua
-- ❌ WRONG: Plays even when step is off
function process_event(track, event)
  note(60, 100, 0)  -- Plays every step!
  return {}
end

-- ✅ CORRECT: Check switch
function process_event(track, event)
  if not event.switch then return {} end
  note(60, 100, 0)
  return {}
end
```

---

## Complete Examples

### Example 1: Simple Drum Machine

```lua
MODE_NAME = "Drums"
SLIDER_LABELS = {"Velocity", "Length", "Accent", "S4"}

-- Drum note assignments (General MIDI)
local drum_map = {
  36,  -- Track 1: Kick
  38,  -- Track 2: Snare
  42,  -- Track 3: Closed Hat
  46,  -- Track 4: Open Hat
  45,  -- Track 5: Low Tom
  47,  -- Track 6: Mid Tom
  50,  -- Track 7: High Tom
  49   -- Track 8: Crash
}

-- Mode 0 context
local velocity_offset = 0

function init(context)
  velocity_offset = context.velocity_offset or 0
  print("Drums initialized, velocity_offset=" .. velocity_offset)
end

function process_event(track, event)
  if not event.switch then return {} end

  -- Get drum note for this track
  local drum_note = drum_map[track + 1]

  -- Get velocity from S1, apply offset
  local velocity = event.pots[1] + velocity_offset
  velocity = math.max(1, math.min(127, velocity))

  -- Get length from S2
  local length = math.max(10, event.pots[2])

  -- Send MIDI
  note(drum_note, velocity, 0)
  off(drum_note, length)

  return {}
end
```

### Example 2: Scale-Aware Melodic Sequencer

```lua
MODE_NAME = "Melody"
SLIDER_LABELS = {"Pitch", "Velocity", "Length", "Pan"}

-- Scale type definitions
local scale_types = {
  {0, 2, 4, 5, 7, 9, 11},    -- Ionian
  {0, 2, 3, 5, 7, 9, 10},    -- Dorian
  {0, 1, 3, 5, 7, 8, 10},    -- Phrygian
  {0, 2, 4, 6, 7, 9, 11},    -- Lydian
  {0, 2, 4, 5, 7, 9, 10},    -- Mixolydian
  {0, 2, 3, 5, 7, 8, 10},    -- Aeolian
  {0, 1, 3, 5, 6, 8, 10},    -- Locrian
  {0, 3, 5, 7, 10}           -- Minor Pentatonic
}

-- Module state
local scale = {0, 2, 4, 5, 7, 9, 11}
local base_note = 60
local velocity_offset = 0

function init(context)
  -- Build scale from Mode 0 context
  local scale_idx = (context.scale_type or 0) + 1
  scale_idx = math.max(1, math.min(#scale_types, scale_idx))
  scale = scale_types[scale_idx]

  -- Set base note with transpose
  base_note = 60 + (context.scale_root or 0)

  -- Store velocity offset
  velocity_offset = context.velocity_offset or 0

  print("Melody: scale_idx=" .. scale_idx .. " base=" .. base_note)
end

function process_event(track, event)
  if not event.switch then return {} end

  -- Map S1 to pitch across 3 octaves
  local s1 = event.pots[1]
  local scale_index = math.floor((s1 * #scale) / 128) + 1
  scale_index = math.max(1, math.min(#scale, scale_index))
  local note_offset = scale[scale_index]

  local octave = math.floor(s1 / 43)
  local octave_shift = octave * 12

  local pitch = base_note + note_offset + octave_shift
  pitch = math.max(0, math.min(127, pitch))

  -- Get velocity from S2, apply offset
  local velocity = event.pots[2] + velocity_offset
  velocity = math.max(1, math.min(127, velocity))

  -- Get length from S3
  local length = 10 + math.floor((event.pots[3] / 127.0) * 490)

  -- Get pan from S4
  local pan = event.pots[4]
  cc(10, pan, 0)

  -- Send note
  note(pitch, velocity, 0)
  off(pitch, length)

  return {}
end
```

### Example 3: Polyphonic Chord Sequencer

```lua
MODE_NAME = "Chords"
SLIDER_LABELS = {"Root", "Type", "Velocity", "Length"}

-- Chord type definitions
local chord_types = {
  {0, 4, 7},           -- Major
  {0, 3, 7},           -- Minor
  {0, 3, 6},           -- Diminished
  {0, 4, 8},           -- Augmented
  {0, 5, 7},           -- Sus4
  {0, 4, 7, 11},       -- Maj7
  {0, 4, 7, 10},       -- Dom7
  {0, 3, 7, 10},       -- Min7
}

-- Mode 0 context
local transpose = 0
local velocity_offset = 0

function init(context)
  transpose = context.scale_root or 0
  velocity_offset = context.velocity_offset or 0
  print("Chords: transpose=" .. transpose .. " vel_offset=" .. velocity_offset)
end

function process_event(track, event)
  if not event.switch then return {} end

  -- S1: Root note
  local root = event.pots[1] + transpose
  root = math.max(0, math.min(127, root))

  -- S2: Chord type
  local chord_idx = math.floor((event.pots[2] / 127.0) * (#chord_types - 1)) + 1
  chord_idx = math.max(1, math.min(#chord_types, chord_idx))
  local chord = chord_types[chord_idx]

  -- S3: Velocity
  local velocity = event.pots[3] + velocity_offset
  velocity = math.max(1, math.min(127, velocity))

  -- S4: Length
  local length = 10 + math.floor((event.pots[4] / 127.0) * 990)

  -- Send all notes in chord
  for i, interval in ipairs(chord) do
    local pitch = root + interval
    pitch = math.max(0, math.min(127, pitch))
    note(pitch, velocity, 0)
    off(pitch, length)
  end

  return {}
end
```

### Example 4: Euclidean Rhythm Generator

```lua
MODE_NAME = "Euclidean"
SLIDER_LABELS = {"Steps", "Pulses", "Velocity", "Pitch"}

-- Module state
local velocity_offset = 0

-- Euclidean rhythm algorithm
function euclidean(steps, pulses)
  if pulses > steps then pulses = steps end
  local pattern = {}
  for i = 1, steps do
    pattern[i] = ((i - 1) * pulses % steps) < pulses
  end
  return pattern
end

function init(context)
  velocity_offset = context.velocity_offset or 0
end

function process_event(track, event)
  if not event.switch then return {} end

  -- S1: Steps (4-16)
  local steps = 4 + math.floor((event.pots[1] / 127.0) * 12)

  -- S2: Pulses (1-steps)
  local pulses = 1 + math.floor((event.pots[2] / 127.0) * (steps - 1))

  -- Generate pattern
  local pattern = euclidean(steps, pulses)

  -- Check if this step should play
  local step_index = (track % steps) + 1
  if not pattern[step_index] then return {} end

  -- S3: Velocity
  local velocity = event.pots[3] + velocity_offset
  velocity = math.max(1, math.min(127, velocity))

  -- S4: Pitch
  local pitch = event.pots[4]
  pitch = math.max(0, math.min(127, pitch))

  note(pitch, velocity, 0)
  off(pitch, 100)

  return {}
end
```

---

## Summary

**Key Points:**
1. **Mode Metadata:** Set `MODE_NAME` and `SLIDER_LABELS` at the top
2. **Context:** Use `init(context)` to receive Mode 0 parameters
3. **Process:** Use `process_event(track, event)` for per-step logic
4. **API:** Call `note()`, `off()`, `cc()` to generate MIDI
5. **Performance:** Pre-calculate in `init()`, keep `process_event()` fast
6. **Scales:** Use Mode 0's scale context for pitched modes
7. **Velocity:** Apply Mode 0's velocity offset to all notes
8. **Testing:** Use desktop build first, debug with `print()` in `init()` only

**Next Steps:**
- Copy `TEMPLATE.lua` to create a new mode
- Refer to `01_drums.lua`, `02_acid.lua`, `03_chords.lua` for real examples
- Test on desktop before deploying to Teensy
- Share your modes with the community!

---

**Happy Mode Making! 🎵**
