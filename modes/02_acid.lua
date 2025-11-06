--[[
  Mode 2: Acid Sequencer

  TB-303 style acid bassline sequencer with slide and filter control.

  Mode Number: 2
  Mode Name: Acid Sequencer
  MIDI Channel: 2

  Description:
  Classic acid bassline sequencer inspired by the Roland TB-303.
  Each track can play a different note with slide and accent.

  Track Usage:
  Tracks 1-8: Different notes in the scale (C, D, E, F, G, A, B, C)

  Slider Mappings:
  S1 (event.pots[1]): Octave (0-127 mapped to octaves 1-4)
  S2 (event.pots[2]): Note length (10-500ms)
  S3 (event.pots[3]): CC Portamento/Slide (0-127)
  S4 (event.pots[4]): CC Filter Cutoff (0-127)

  Button Interaction:
  B1-B16: Toggle note on/off for current track
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
  local midi = {}

  -- Only play if switch is on
  if not event.switch then
    return midi
  end

  -- Map track to scale degree (0-7 tracks, we'll use first 5 notes repeatedly)
  local scale_degree = (track % #scale) + 1
  local note_offset = scale[scale_degree]

  -- Map octave from S1 (0-127 -> octaves 1-4)
  local octave_shift = math.floor(event.pots[1] / 32) * 12  -- 0, 12, 24, or 36

  -- Calculate final pitch
  local pitch = base_note + note_offset + octave_shift

  -- Clamp to valid MIDI range
  pitch = math.max(0, math.min(127, pitch))

  -- Get velocity (fixed at 100 for acid, or could use accent)
  local velocity = 100

  -- Get note length from S2 (map 0-127 to 10-500ms)
  local note_length = 10 + math.floor((event.pots[2] / 127.0) * 490)

  -- Send note on
  table.insert(midi, note(pitch, velocity, 0))

  -- Send filter cutoff CC (74 = brightness/cutoff)
  local filter_cutoff = event.pots[4]
  table.insert(midi, cc(74, filter_cutoff, 0))

  -- Send portamento/slide CC (5 = portamento time, 65 = portamento on/off)
  local portamento_amount = event.pots[3]
  if portamento_amount > 20 then
    -- Enable portamento
    table.insert(midi, cc(65, 127, 0))  -- Portamento on
    table.insert(midi, cc(5, portamento_amount, 0))  -- Portamento time
  else
    -- Disable portamento
    table.insert(midi, cc(65, 0, 0))  -- Portamento off
  end

  -- Send note off after specified length
  table.insert(midi, off(pitch, note_length))

  return midi
end
