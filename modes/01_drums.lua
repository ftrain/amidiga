--[[
  Mode 1: Drum Machine

  808-style beat machine with 8 drum sounds across 8 tracks.

  Track Usage:
  Track 1: Kick
  Track 2: Snare
  Track 3: Closed Hi-Hat
  Track 4: Open Hi-Hat
  Track 5: Low Tom
  Track 6: Mid Tom
  Track 7: High Tom
  Track 8: Accent (boosts velocity of all active notes)

  Slider Mappings:
  S1 (pots[1]): Velocity/volume (0-127)
  S2 (pots[2]): Note length (1-127ms)
  S3 (pots[3]): Unused
  S4 (pots[4]): Unused

  Button Interaction:
  B1-B16: Toggle beats on/off for current track
]]--

-- MIDI note assignments for drum sounds (General MIDI Drum Map)
local drum_map = {
  36,  -- Track 1: Kick (C1)
  38,  -- Track 2: Snare (D1)
  42,  -- Track 3: Closed Hi-Hat (F#1)
  46,  -- Track 4: Open Hi-Hat (A#1)
  45,  -- Track 5: Low Tom (A1)
  47,  -- Track 6: Mid Tom (B1)
  50,  -- Track 7: High Tom (D2)
  0    -- Track 8: Accent (no note, just modifies others)
}

local accent_track = 7  -- Track 8 (0-indexed as 7)
local accent_boost = 30  -- Velocity boost when accent is active

function init(context)
  print("Drum machine initialized on channel " .. context.midi_channel)
end

function process_event(track, event)
  -- Skip if switch is not on
  if not event.switch then
    return {}
  end

  -- Track 8 is accent - doesn't play a note
  if track == accent_track then
    return {}
  end

  -- Get drum note for this track
  local drum_note = drum_map[track + 1]  -- Lua is 1-indexed

  -- Get velocity from S1
  local velocity = event.pots[1]

  -- Apply accent if track 8 is also active (we don't have cross-track info here,
  -- so this is simplified - just use pot value as base velocity)
  velocity = math.min(127, velocity)

  -- Get note length from S2 (minimum 10ms)
  local note_length = math.max(10, event.pots[2])

  -- Send MIDI events (these directly add to internal buffer)
  note(drum_note, velocity, 0)
  off(drum_note, note_length)

  return {}  -- Return value is ignored; events are in internal buffer
end
