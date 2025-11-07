--[[
  Mode 8: Drunk Sequencer

  Notes wobble around their programmed pitch like a drunk person walking.
  Each track maintains a "drunk offset" that randomly walks up and down.

  Sliders:
  - S1: Base pitch (MIDI note)
  - S2: Drunk amount (how far notes can wander, 0-127 -> 0-12 semitones)
  - S3: Coherence (return-to-center force, higher = more coherent)
  - S4: Velocity chaos (0 = stable, 127 = maximum random velocity)
]]--

MODE_NAME = "Drunk Sequencer"

-- Persistent drunk offset for each track (-12 to +12 semitones)
local drunk_offset = {0, 0, 0, 0, 0, 0, 0, 0}

-- Random seed based on step counter (for deterministic randomness)
local step_counter = 0

function init(context)
    print("Drunk Sequencer initialized on channel " .. context.midi_channel)
    math.randomseed(os.time())
end

function process_event(track, event)
    step_counter = step_counter + 1

    if not event.switch then
        return
    end

    -- Map S2 to max drunk range (0-127 -> 0-12 semitones)
    local max_drunk = math.floor((event.pots[2] * 12) / 127)

    -- Map S3 to coherence (0-127 -> 0-1, higher = more pull to center)
    local coherence = event.pots[3] / 127.0

    -- Random walk with coherence
    local walk_step = math.random(-2, 2)
    local center_pull = -drunk_offset[track + 1] * coherence * 0.3
    drunk_offset[track + 1] = drunk_offset[track + 1] + walk_step + center_pull

    -- Clamp to max drunk range
    drunk_offset[track + 1] = math.max(-max_drunk, math.min(max_drunk, drunk_offset[track + 1]))

    -- Base pitch from S1, add drunk offset
    local base_pitch = event.pots[1]
    local final_pitch = base_pitch + drunk_offset[track + 1]
    final_pitch = math.max(0, math.min(127, final_pitch))  -- Clamp to MIDI range

    -- Map S4 to velocity chaos (0 = use programmed velocity, 127 = full random)
    local chaos_amount = event.pots[4] / 127.0
    local base_velocity = 100
    local velocity_chaos = math.random(-30, 30) * chaos_amount
    local velocity = math.floor(base_velocity + velocity_chaos)
    velocity = math.max(1, math.min(127, velocity))

    -- Note length: shorter for drunk notes, longer for sober
    local note_length = 80 + math.floor((1.0 - chaos_amount) * 100)

    -- Play the drunk note
    note(final_pitch, velocity)
    off(final_pitch, note_length)
end
