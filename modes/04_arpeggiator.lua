MODE_NAME = "Arp"
SLIDER_LABELS = {"Root", "Pattern", "Velocity", "Length"}


--[[
  Mode 4: Arpeggiator

  Arpeggiated chord patterns with multiple playback styles.

  Mode Number: 4
  Mode Name: Arpeggiator
  MIDI Channel: 4

  Description:
  Plays arpeggiated patterns based on chord types. Each track plays
  a different step of the arpeggio, creating complex polyrhythmic patterns.

  Track Usage:
  Track 0-7: Each track represents a different step in the arpeggio cycle

  Slider Mappings:
  S1 (event.pots[1]): Root note (MIDI note number, 0-127)
  S2 (event.pots[2]): Arp pattern type (0-127 mapped to 8 patterns)
  S3 (event.pots[3]): Velocity (0-127)
  S4 (event.pots[4]): Note length (10-500ms)

  Button Interaction:
  B1-B16: Toggle notes on/off for each step
]]--

-- ============================================================================
-- Module-level variables
-- ============================================================================

-- Arpeggio patterns (intervals from root)
local arp_patterns = {
  {0, 4, 7},              -- Major triad up
  {0, 3, 7},              -- Minor triad up
  {0, 4, 7, 12},          -- Major octave up
  {0, 3, 7, 10},          -- Minor 7th up
  {0, 7, 12, 19},         -- Fifths cascade
  {0, 12, 7, 12},         -- Octave bounce
  {0, 4, 7, 4},           -- Major up-down
  {0, 2, 4, 5, 7, 9, 11}  -- Major scale run
}


-- ============================================================================
-- init() - Called once when mode loads
-- ============================================================================

function init(context)
  -- Nothing special needed for arp mode
end


-- ============================================================================
-- process_event() - Called for each Event during playback
-- ============================================================================

function process_event(track, event)
  -- Only play if switch is on
  if not event.switch then
    return {}
  end

  -- Get parameters
  local root_note = event.pots[1]  -- S1: Root note (0-127)
  local pattern_select = event.pots[2]  -- S2: Pattern type
  local velocity = event.pots[3]  -- S3: Velocity
  local note_length = 10 + math.floor((event.pots[4] / 127.0) * 490)  -- S4: Length (10-500ms)

  -- Clamp root note to valid MIDI range
  root_note = math.max(24, math.min(96, root_note))

  -- Select arp pattern (0-127 maps to 0-7)
  local pattern_index = math.floor(pattern_select / 16) + 1
  pattern_index = math.max(1, math.min(#arp_patterns, pattern_index))
  local pattern = arp_patterns[pattern_index]

  -- Each track plays a different note in the arpeggio
  local arp_step = (track % #pattern) + 1
  local interval = pattern[arp_step]
  local pitch = root_note + interval

  -- Clamp to valid MIDI range
  pitch = math.max(0, math.min(127, pitch))

  -- Send MIDI note
  note(pitch, velocity, 0)
  off(pitch, note_length)

  return {}
end
