MODE_NAME = "Song"

--[[
  Mode 0: Song/Pattern Sequencer

  Pattern chain mode - sequences which patterns play on all other modes.

  Mode Number: 0
  Mode Name: Song
  MIDI Channel: 0 (no MIDI output from this mode)

  Description:
  Mode 0 doesn't output MIDI notes itself. Instead, it controls which
  pattern gets played across ALL other modes (1-14). This lets you create
  pattern chains and song arrangements.

  How It Works:
  - Each step on Track 0 specifies which pattern to play
  - S1 encodes the pattern number (0-31)
  - When step is active, all modes switch to that pattern
  - Inactive steps don't change the pattern

  Track Usage:
  Track 0: Pattern sequencer (other tracks unused)

  Slider Mappings:
  S1 (event.pots[1]): Pattern number (0-127 mapped to patterns 0-31)
  S2-S4: Unused in this mode

  Button Interaction:
  B1-B16: Toggle pattern change at each step

  Example Use:
  Step 0: Pattern 0 (intro)
  Step 4: Pattern 1 (verse)
  Step 8: Pattern 2 (chorus)
  Step 12: Pattern 3 (bridge)
]]--

-- ============================================================================
-- init() - Called once when mode loads
-- ============================================================================

function init(context)
  print("Song/Pattern sequencer initialized on channel " .. context.midi_channel)
  print("Mode 0 controls pattern playback for all other modes")
end


-- ============================================================================
-- process_event() - Called for each Event during playback
-- ============================================================================

function process_event(track, event)
  -- Mode 0 doesn't generate MIDI output
  -- Pattern switching is handled by the C++ engine
  -- This function exists for consistency but does nothing

  return {}
end
