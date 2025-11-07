MODE_NAME = "Song"

--[[
  Mode 0: Song/Pattern Sequencer

  Pattern chain mode - sequences which patterns play on all other modes.

  **IMPORTANT: Mode 0 runs at 1/16th speed**
  Each step in Mode 0 lasts for 16 regular steps (one full pattern).
  This means Mode 0 has 16 steps that each select a pattern for a full bar.

  Mode Number: 0
  Mode Name: Song
  MIDI Channel: 0 (no MIDI output from this mode)

  Description:
  Mode 0 doesn't output MIDI notes itself. Instead, it controls which
  pattern gets played across ALL other modes (1-14). This lets you create
  pattern chains and song arrangements.

  How It Works:
  - Mode 0 runs at 1/16th speed (each step = 1 full 16-step pattern)
  - Each of the 16 steps in Mode 0 specifies which pattern to play
  - S1 encodes the pattern number (displayed as 1-32, internally 0-31)
  - When step is active, all modes switch to that pattern for the full bar
  - Inactive steps keep playing the previous pattern

  Track Usage:
  Track 0: Pattern sequencer (other tracks unused)

  Slider Mappings:
  S1 (event.pots[1]): Pattern number (0-127 mapped to patterns 1-32)
  S2-S4: Unused in this mode

  Button Interaction:
  B1-B16: Toggle pattern change at each "song step" (every 16 regular steps)

  Default Pattern:
  Steps 0-3:  Pattern 1 (displayed as pattern 1)
  Steps 4-7:  Pattern 2 (displayed as pattern 2)
  Steps 8-11: Pattern 3 (displayed as pattern 3)
  Steps 12-15: Pattern 4 (displayed as pattern 4)
  This creates a 4-pattern song that repeats: 1 2 3 4

  Example Use:
  Step 0-3: Pattern 1 (intro, plays for 4 bars = 64 steps)
  Step 4-7: Pattern 2 (verse, plays for 4 bars = 64 steps)
  Step 8-11: Pattern 3 (chorus, plays for 4 bars = 64 steps)
  Step 12-15: Pattern 4 (bridge, plays for 4 bars = 64 steps)
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
