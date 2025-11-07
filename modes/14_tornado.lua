--[[
  Mode 14: Tornado (Spiral Sequencer)

  Notes spiral through pitch space like a tornado, creating hypnotic
  rising/falling patterns with Shepard tone-like qualities.

  Sliders:
  - S1: Spiral radius (pitch range, 0 = tight spiral, 127 = wide)
  - S2: Spiral speed (rotation speed, 0 = slow, 127 = fast)
  - S3: Rising speed (pitch climb per rotation, can be negative)
  - S4: Chaos (random deviation from perfect spiral)

  Each track represents a different position on the spiral helix.
  Creates infinite-rising illusion when parameters are tuned right.
]]--

MODE_NAME = "Tornado"

-- Spiral angle for each track (in radians)
local angle = {0, 0.785, 1.57, 2.356, 3.14, 3.927, 4.712, 5.498}  -- 8 positions around circle

-- Vertical position (pitch height) for each track
local height = {0, 4, 8, 12, 16, 20, 24, 28}

-- Step counter for global spiral evolution
local step_count = 0

function init(context)
    print("Tornado Spiral Sequencer initialized on channel " .. context.midi_channel)
    math.randomseed(os.time())
end

-- Shepard tone layer selection (for infinite rise illusion)
function shepard_layer(base_pitch, height)
    -- Wrap pitch to stay within MIDI range while creating illusion of infinite rise
    local wrapped = base_pitch + height
    while wrapped > 100 do
        wrapped = wrapped - 24  -- Drop 2 octaves but keep spiral going
    end
    while wrapped < 36 do
        wrapped = wrapped + 24  -- Rise 2 octaves
    end
    return wrapped
end

function process_event(track, event)
    step_count = step_count + 1

    if not event.switch then
        return
    end

    -- Map S1 to spiral radius (0-127 -> 0-12 semitones)
    local radius = (event.pots[1] / 127.0) * 12

    -- Map S2 to spiral speed (0-127 -> 0.1 to 1.0 radians per step)
    local speed = 0.1 + (event.pots[2] / 127.0) * 0.9

    -- Map S3 to rising speed (0-127 -> -2.0 to +2.0 semitones per rotation)
    -- 64 = no rise, < 64 = descending, > 64 = ascending
    local rise_speed = ((event.pots[3] / 127.0) - 0.5) * 4.0

    -- Map S4 to chaos (0-127 -> 0.0 to 1.0)
    local chaos = event.pots[4] / 127.0

    -- Advance spiral angle
    angle[track + 1] = angle[track + 1] + speed

    -- Calculate position on spiral
    -- X-axis (not used for MIDI, but conceptually)
    local x = math.cos(angle[track + 1]) * radius

    -- Y-axis translates to pitch offset
    local y = math.sin(angle[track + 1]) * radius

    -- Advance vertical height (rising/falling)
    height[track + 1] = height[track + 1] + rise_speed * (speed / 0.5)

    -- Base pitch (center of spiral)
    local base_pitch = 60 + track * 2  -- Each track offset

    -- Calculate spiral pitch
    local spiral_pitch = base_pitch + y + (height[track + 1] / 4)

    -- Add chaos (random deviation)
    if chaos > 0 then
        local chaos_offset = (math.random() - 0.5) * 12 * chaos
        spiral_pitch = spiral_pitch + chaos_offset
    end

    -- Apply Shepard tone wrapping for infinite rise
    local final_pitch = shepard_layer(spiral_pitch, height[track + 1])
    final_pitch = math.floor(final_pitch)
    final_pitch = math.max(0, math.min(127, final_pitch))

    -- Velocity based on position in spiral (higher = louder)
    local velocity_mod = (math.sin(angle[track + 1]) + 1.0) / 2.0  -- 0.0 to 1.0
    local velocity = 60 + math.floor(velocity_mod * 40)
    velocity = math.max(40, math.min(100, velocity))

    -- Note length varies with spiral position (creates phrasing)
    local length_mod = (math.cos(angle[track + 1] * 0.5) + 1.0) / 2.0
    local note_length = 50 + math.floor(length_mod * 100)

    -- Play tornado note
    note(final_pitch, velocity)
    off(final_pitch, note_length)

    -- Add subtle filter sweep following spiral
    local filter_value = math.floor(((math.sin(angle[track + 1] * 0.25) + 1.0) / 2.0) * 127)
    cc(74, filter_value)  -- Filter cutoff follows spiral

    -- Extra harmonic layer on peaks (when at top of spiral wave)
    if velocity_mod > 0.8 and math.random() < 0.4 then
        local harmonic = final_pitch + 12  -- Octave up
        if harmonic <= 127 then
            note(harmonic, velocity // 2, 20)  -- Slightly delayed
            off(harmonic, note_length - 20)
        end
    end
end
