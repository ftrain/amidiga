MODE_NAME = "Acid"
SLIDER_LABELS = {"Pitch", "Length", "Slide", "Filter"}


--[[
  Mode 2: Acid Sequencer

  TB-303 style acid bassline sequencer with slide and filter control.

  Mode Number: 2
  Mode Name: Acid Sequencer
  MIDI Channel: 2

  Description:
  Classic acid bassline sequencer inspired by the Roland TB-303.
  Single-track melodic sequencer where each step's pitch is set by S1.

  Track Usage:
  Track 1 (or any single track): Main acid bassline

  Slider Mappings:
  S1 (event.pots[1]): Note pitch (0-127 mapped across 3 octaves of scale)
  S2 (event.pots[2]): Note length (10-500ms)
  S3 (event.pots[3]): CC Portamento/Slide (0-127)
  S4 (event.pots[4]): CC Filter Cutoff (0-127)

  Button Interaction:
  B1-B16: Toggle note on/off for each step

  Mode 0 Context:
  - scale_root: Transposes the scale (0-11 = C-B)
  - scale_type: Chooses scale type (0-7)
  - velocity_offset: Adjusts velocity
]]--

-- ============================================================================
-- Module-level variables
-- ============================================================================

-- Scale type definitions (intervals in semitones)
local scale_types = {
  {0, 2, 4, 5, 7, 9, 11},    -- 0: Ionian (Major)
  {0, 2, 3, 5, 7, 9, 10},    -- 1: Dorian
  {0, 1, 3, 5, 7, 8, 10},    -- 2: Phrygian
  {0, 2, 4, 6, 7, 9, 11},    -- 3: Lydian
  {0, 2, 4, 5, 7, 9, 10},    -- 4: Mixolydian
  {0, 2, 3, 5, 7, 8, 10},    -- 5: Aeolian (Natural Minor)
  {0, 1, 3, 5, 6, 8, 10},    -- 6: Locrian
  {0, 3, 5, 7, 10}           -- 7: Minor Pentatonic (acid classic)
}

local scale_names = {
  "Ionian", "Dorian", "Phrygian", "Lydian",
  "Mixolydian", "Aeolian", "Locrian", "Min Pent"
}

-- Mode 0 context (set by init)
local scale = {0, 3, 5, 7, 10}  -- Default: Minor pentatonic
local base_note = 36  -- Default: C2
local velocity_offset = 0


-- ============================================================================
-- init() - Called once when mode loads
-- ============================================================================

function init(context)
  -- Get Mode 0 scale context
  local scale_root = context.scale_root or 0  -- 0-11 (C-B)
  local scale_type_idx = (context.scale_type or 7) + 1  -- Convert to 1-indexed, default to pentatonic
  scale_type_idx = math.max(1, math.min(#scale_types, scale_type_idx))

  -- Build scale from Mode 0 context
  scale = scale_types[scale_type_idx]
  base_note = 36 + scale_root  -- C2 + transpose

  -- Store velocity offset
  velocity_offset = context.velocity_offset or 0

  print("Acid initialized: " .. scale_names[scale_type_idx] .. " root=" .. scale_root .. " velocity_offset=" .. velocity_offset)
end


-- ============================================================================
-- process_event() - Called for each Event during playback
-- ============================================================================

function process_event(track, event)
  -- Only play if switch is on
  if not event.switch then
    return {}
  end

  -- Map S1 (0-127) to note pitch across 3 octaves of the scale
  local s1_value = event.pots[1]

  -- Determine which scale degree
  local scale_index = math.floor((s1_value * #scale) / 128) + 1
  scale_index = math.max(1, math.min(#scale, scale_index))
  local note_offset = scale[scale_index]

  -- Determine octave (0-127 gives us roughly 3 octaves)
  local octave = math.floor(s1_value / 43)  -- 128 / 3 ≈ 43
  local octave_shift = octave * 12

  -- Calculate final pitch
  local pitch = base_note + note_offset + octave_shift

  -- Clamp to valid MIDI range
  pitch = math.max(0, math.min(127, pitch))

  -- Get velocity (fixed at 100 for acid, plus offset)
  local velocity = 100 + velocity_offset
  velocity = math.max(1, math.min(127, velocity))

  -- Get note length from S2 (map 0-127 to 10-500ms)
  local note_length = 10 + math.floor((event.pots[2] / 127.0) * 490)

  -- Send MIDI events (these directly add to internal buffer)
  note(pitch, velocity, 0)

  -- Send filter cutoff CC (74 = brightness/cutoff)
  local filter_cutoff = event.pots[4]
  cc(74, filter_cutoff, 0)

  -- Send portamento/slide CC (5 = portamento time, 65 = portamento on/off)
  local portamento_amount = event.pots[3]
  if portamento_amount > 20 then
    -- Enable portamento
    cc(65, 127, 0)  -- Portamento on
    cc(5, portamento_amount, 0)  -- Portamento time
  else
    -- Disable portamento
    cc(65, 0, 0)  -- Portamento off
  end

  -- Send note off after specified length
  off(pitch, note_length)

  return {}  -- Return value is ignored; events are in internal buffer
end
