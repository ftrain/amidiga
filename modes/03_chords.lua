MODE_NAME = "Chords"


--[[
  Mode 3: Chord Sequencer

  Polyphonic chord sequencer with multiple chord types and voicings.

  Mode Number: 3
  Mode Name: Chords
  MIDI Channel: 3

  Description:
  Trigger chords with different qualities and voicings. Each step can have
  a different chord with parameter-locked root note and chord type.

  Track Usage:
  All tracks can trigger chords independently

  Slider Mappings:
  S1 (event.pots[1]): Root Note (0-127 mapped to MIDI notes C0-G10)
  S2 (event.pots[2]): Chord Type (0-127 mapped to 16 chord types)
  S3 (event.pots[3]): Velocity (0-127)
  S4 (event.pots[4]): Note Length (10-1000ms)

  Button Interaction:
  B1-B16: Toggle chord trigger for current track
]]--

-- ============================================================================
-- Module-level variables
-- ============================================================================

-- Chord definitions (intervals in semitones from root)
local chord_types = {
  {0, 4, 7},           -- 1. Major triad
  {0, 3, 7},           -- 2. Minor triad
  {0, 3, 6},           -- 3. Diminished
  {0, 4, 8},           -- 4. Augmented
  {0, 5, 7},           -- 5. Sus4
  {0, 2, 7},           -- 6. Sus2
  {0, 4, 7, 11},       -- 7. Major 7th
  {0, 4, 7, 10},       -- 8. Dominant 7th
  {0, 3, 7, 10},       -- 9. Minor 7th
  {0, 3, 6, 9},        -- 10. Diminished 7th
  {0, 4, 8, 11},       -- 11. Augmented 7th
  {0, 3, 7, 11},       -- 12. Minor major 7th
  {0, 4, 7, 9},        -- 13. Major 6th
  {0, 3, 7, 9},        -- 14. Minor 6th
  {0, 5, 7, 10},       -- 15. 7sus4
  {0, 7},              -- 16. Power chord (root + fifth)
}

local chord_names = {
  "Maj", "Min", "Dim", "Aug", "Sus4", "Sus2",
  "Maj7", "Dom7", "Min7", "Dim7", "Aug7", "MinMaj7",
  "Maj6", "Min6", "7sus4", "Power"
}

-- ============================================================================
-- init() - Called once when mode loads
-- ============================================================================

function init(context)
  print("Chord sequencer initialized on channel " .. context.midi_channel)
end


-- ============================================================================
-- process_event() - Called for each Event during playback
-- ============================================================================

function process_event(track, event)
  -- Skip if switch is not on
  if not event.switch then
    return {}
  end

  -- Get root note from S1 (0-127 maps to full MIDI range)
  local root_note = event.pots[1]

  -- Get chord type from S2 (0-127 mapped to 16 chord types)
  local chord_index = math.floor((event.pots[2] / 127.0) * (#chord_types - 1)) + 1
  chord_index = math.max(1, math.min(#chord_types, chord_index))
  local chord = chord_types[chord_index]

  -- Get velocity from S3
  local velocity = math.max(1, math.min(127, event.pots[3]))

  -- Get note length from S4 (map 0-127 to 10-1000ms)
  local note_length = 10 + math.floor((event.pots[4] / 127.0) * 990)

  -- Send all notes in the chord
  for i, interval in ipairs(chord) do
    local pitch = root_note + interval

    -- Clamp to valid MIDI range
    pitch = math.max(0, math.min(127, pitch))

    -- Send note on
    note(pitch, velocity, 0)

    -- Send note off after specified length
    off(pitch, note_length)
  end

  return {}  -- Return value is ignored; events are in internal buffer
end
