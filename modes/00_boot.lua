--[[
  Mode 0: Boot Mode

  This mode handles loading, saving, and erasing songs.

  Button patterns:
  - Tap B1 once: Load song
  - Tap B1 twice (quickly): Save song
  - Tap B1 once + long press: Erase song

  For now, this is a stub that just prints messages.
]]--

function init(context)
  print("Boot mode initialized")
  print("Mode: " .. context.mode_number .. ", Tempo: " .. context.tempo .. " BPM")
end

function process_event(track, event)
  -- Boot mode doesn't generate MIDI events
  -- In a full implementation, this would detect button patterns
  -- and call load/save/erase functions

  return {}
end
