--[[
  Mode 12: Lunar Phase Sequencer

  Musical patterns that evolve like moon phases through a 28-step cycle.
  Velocity and probability mapped to sinusoidal lunar phase.

  Sliders:
  - S1: Phase speed (how fast to cycle through phases, 0 = slow, 127 = fast)
  - S2: Full moon brightness (max velocity at peak, 0-127)
  - S3: New moon silence (probability of silence at trough, 0-100%)
  - S4: Base pitch

  Creates long-form evolving patterns with natural ebb and flow.
  Each track is offset by π/4 radians (different moon phase).
]]--

MODE_NAME = "Lunar Phase"
SLIDER_LABELS = {"Speed", "Bright", "Silent", "Pitch"}

-- Phase counter (0 to 28 for each track)
local phase = {0, 3.5, 7, 10.5, 14, 17.5, 21, 24.5}  -- Offset phases

-- Global step counter
local step_count = 0

function init(context)
    print("Lunar Phase Sequencer initialized on channel " .. context.midi_channel)
end

-- Calculate moon phase value (0.0 to 1.0, where 0.5 = full moon)
function moon_value(current_phase)
    -- Use sine wave: 0 = new moon, 0.5 = full moon, 1.0 = new moon again
    local normalized = (current_phase % 28) / 28.0
    return (math.sin(normalized * 2 * math.pi - math.pi/2) + 1.0) / 2.0
end

function process_event(track, event)
    step_count = step_count + 1

    if not event.switch then
        return
    end

    -- Map S1 to phase speed (0-127 -> 0.1 to 2.0 steps per event)
    local speed = 0.1 + (event.pots[1] / 127.0) * 1.9

    -- Advance this track's phase
    phase[track + 1] = phase[track + 1] + speed

    -- Wrap at 28
    if phase[track + 1] >= 28 then
        phase[track + 1] = phase[track + 1] - 28
    end

    -- Calculate current moon phase value (0.0 to 1.0)
    local moon = moon_value(phase[track + 1])

    -- Map S3 to new moon silence threshold
    local silence_threshold = event.pots[3] / 127.0

    -- During new moon (moon < 0.3), apply silence probability
    if moon < 0.3 then
        local silence_prob = (0.3 - moon) / 0.3 * silence_threshold
        if math.random() < silence_prob then
            return  -- Silent (new moon darkness)
        end
    end

    -- Map S2 to full moon brightness (max velocity)
    local max_velocity = event.pots[2]
    if max_velocity < 20 then max_velocity = 127 end

    -- Velocity follows moon phase
    local velocity = math.floor(max_velocity * moon)
    velocity = math.max(20, velocity)

    -- Base pitch from S4, modulate by moon phase
    local base_pitch = event.pots[4]
    -- Slight pitch drift with phases (full moon = higher, new moon = lower)
    local pitch_offset = math.floor((moon - 0.5) * 4)
    local pitch = base_pitch + pitch_offset + track
    pitch = math.max(0, math.min(127, pitch))

    -- Note length varies with phase (fuller moon = longer notes)
    local note_length = math.floor(50 + moon * 150)

    -- Play the lunar note
    note(pitch, velocity)
    off(pitch, note_length)

    -- Add subtle CC modulation (like tides)
    local tide_value = math.floor(moon * 127)
    cc(1, tide_value)  -- Modulation wheel follows moon phase
end
