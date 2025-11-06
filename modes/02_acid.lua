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
]]--

-- ============================================================================
-- Module-level variables
-- ============================================================================

-- Minor pentatonic scale (classic acid sound)
local scale = {0, 3, 5, 7, 10}  -- C, Eb, F, G, Bb

-- Base MIDI note (C2)
local base_note = 36


-- ============================================================================
-- init() - Called once when mode loads
-- ============================================================================

function init(context)
  -- Nothing special needed for acid mode
end


-- ============================================================================
-- process_event() - Called for each Event during playback
-- ============================================================================

function process_event(track, event)
  -- Only play if switch is on
  if not event.switch then
    return {}
  end

  -- Map S1 (0-127) to note pitch across 3 octaves of the pentatonic scale
  -- Each unit of scale spans ~25 values, giving us about 5 steps per octave
  local s1_value = event.pots[1]

  -- Determine which scale degree (0-4 for pentatonic)
  local scale_index = math.floor((s1_value % 42) / 8.4)  -- Maps 0-42 to 0-4
  scale_index = math.max(1, math.min(#scale, scale_index + 1))
  local note_offset = scale[scale_index]

  -- Determine octave (0-127 gives us 3 octaves)
  local octave = math.floor(s1_value / 42)  -- 0, 1, or 2
  local octave_shift = octave * 12

  -- Calculate final pitch
  local pitch = base_note + note_offset + octave_shift

  -- Clamp to valid MIDI range
  pitch = math.max(0, math.min(127, pitch))

  -- Get velocity (fixed at 100 for acid, or could use accent)
  local velocity = 100

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
