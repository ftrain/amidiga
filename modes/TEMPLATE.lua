-- ============================================================================
-- Mode Metadata (required)
-- ============================================================================

MODE_NAME = "[Your Mode Name]"

-- Slider labels (optional - provides GUI labels for S1-S4)
SLIDER_LABELS = {"[S1 Label]", "[S2 Label]", "[S3 Label]", "[S4 Label]"}

--[[
  GRUVBOK Mode Template

  Copy this file to create a new mode:
  1. Copy to XX_modename.lua (e.g., 03_arpeggiator.lua)
  2. Fill in the description below
  3. Implement init() and process_event()
  4. Test on desktop before deploying to Teensy

  Mode Number: XX
  Mode Name: [Your Mode Name]
  MIDI Channel: XX (mode number)

  Description:
  [Describe what this mode does musically]

  Track Usage:
  Track 1: [What does track 1 do?]
  Track 2: [What does track 2 do?]
  ...
  Track 8: [What does track 8 do?]

  Slider Mappings:
  S1 (event.pots[1]): [Parameter name, range, purpose]
  S2 (event.pots[2]): [Parameter name, range, purpose]
  S3 (event.pots[3]): [Parameter name, range, purpose]
  S4 (event.pots[4]): [Parameter name, range, purpose]

  Button Interaction:
  B1-B16: [What do the switches do? Note triggers? Step enables?]

  Mode 0 Context (applied globally from master control):
  - scale_root: Global scale root (0-11 = C-B), use for transposing pitched content
  - scale_type: Global scale type (0-7 = Ionian/Dorian/etc), use for building scales
  - velocity_offset: Per-mode velocity offset (-64 to +63), apply to all note velocities
]]--

-- ============================================================================
-- Module-level variables (state that persists across events)
-- ============================================================================

-- Example: local scale = {0, 2, 4, 5, 7, 9, 11}  -- Major scale
-- Example: local base_note = 60  -- Middle C
-- Example: local velocity_offset = 0  -- Store Mode 0 velocity offset


-- ============================================================================
-- init() - Called once when mode loads
-- ============================================================================
--
-- Parameters:
--   context (table):
--     .tempo           - Current tempo in BPM
--     .mode_number     - This mode's number (0-15)
--     .midi_channel    - MIDI channel for this mode
--     .scale_root      - Global scale root (0-11 = C-B) from Mode 0
--     .scale_type      - Global scale type (0-7) from Mode 0
--     .velocity_offset - Per-mode velocity offset (-64 to +63) from Mode 0
--
-- Use this to:
--   - Initialize module-level variables
--   - Set up lookup tables based on Mode 0 scale context
--   - Prepare any state needed for processing
--
function init(context)
  -- Example: print("Mode " .. context.mode_number .. " initialized at " .. context.tempo .. " BPM")

  -- Example: Store Mode 0 context for use in process_event()
  -- velocity_offset = context.velocity_offset or 0

  -- Example: Build scale from Mode 0 context
  -- local scale_types = {
  --   {0, 2, 4, 5, 7, 9, 11},  -- Ionian
  --   {0, 2, 3, 5, 7, 9, 10},  -- Dorian
  --   -- ... etc
  -- }
  -- local scale_idx = (context.scale_type or 0) + 1
  -- scale = scale_types[scale_idx]
  -- base_note = 60 + (context.scale_root or 0)

  -- TODO: Add your initialization code here
end


-- ============================================================================
-- process_event() - Called for each Event during playback
-- ============================================================================
--
-- Parameters:
--   track (number): Track number (0-7)
--   event (table):
--     .switch      - Boolean: true if button pressed, false if not
--     .pots        - Table of 4 values: {pot1, pot2, pot3, pot4} (0-127)
--
-- Returns:
--   Array of MIDI events (table)
--
-- Available API functions:
--   note(pitch, velocity, [delta])  - Send note on (delta in ms, default 0)
--   off(pitch, [delta])             - Send note off (delta in ms, default 0)
--   cc(controller, value, [delta])  - Send control change
--   stopall([delta])                - All notes off
--
function process_event(track, event)
  -- TODO: Implement your event processing logic

  -- NOTE: The API functions (note, off, cc, stopall) directly add events
  -- to an internal buffer. Don't try to collect their return values.
  -- Just call them and they'll be scheduled automatically.

  -- Example: Simple note trigger
  --[[
  if event.switch then
    local pitch = 60 + track  -- C4 to G4 across tracks
    local velocity = event.pots[1]  -- S1 controls velocity

    note(pitch, velocity, 0)  -- Send note on immediately
    off(pitch, 100)           -- Send note off after 100ms
  end
  ]]--

  -- Example: Control change
  --[[
  if event.switch then
    -- Send CC for filter cutoff
    cc(74, event.pots[4], 0)
  end
  ]]--

  return {}  -- Return value is ignored
end


-- ============================================================================
-- Helper functions (optional)
-- ============================================================================

-- Add any helper functions you need
-- Examples:

--[[
-- Map a value from one range to another
function map(value, in_min, in_max, out_min, out_max)
  return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min
end

-- Quantize note to scale
function quantize_to_scale(note, scale)
  local octave = math.floor(note / 12)
  local pitch_class = note % 12

  -- Find closest scale degree
  local closest = scale[1]
  local min_dist = math.abs(pitch_class - scale[1])

  for i = 2, #scale do
    local dist = math.abs(pitch_class - scale[i])
    if dist < min_dist then
      closest = scale[i]
      min_dist = dist
    end
  end

  return octave * 12 + closest
end

-- Generate euclidean rhythm
function euclidean(steps, pulses)
  local pattern = {}
  for i = 1, steps do
    pattern[i] = (i - 1) * pulses % steps < pulses
  end
  return pattern
end
]]--


-- ============================================================================
-- Notes for Mode Developers
-- ============================================================================

--[[
  Tips:
  1. Keep process_event() FAST - it's called for every step
  2. Avoid allocations in hot paths if possible
  3. Use module-level variables for state that persists
  4. Test on desktop before deploying to Teensy
  5. Remember: pot values are 0-127 (MIDI range)
  6. Delta times are in milliseconds

  Common Patterns:

  - Velocity control: velocity = event.pots[1]
  - Octave shift: pitch = base_pitch + (event.pots[2] // 16) * 12
  - Note length: table.insert(midi_events, off(pitch, event.pots[3]))
  - Filter CC: table.insert(midi_events, cc(74, event.pots[4]))

  Debugging:
  - Use print() to debug (shows in console on desktop)
  - Check MIDI output with MIDI monitor tool
  - Verify delta timing doesn't overlap incorrectly
]]--
