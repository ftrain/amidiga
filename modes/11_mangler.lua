--[[
  Mode 11: MIDI Mangler

  Destroys and glitches MIDI patterns with various digital effects.
  Creates controlled chaos from programmed sequences.

  Sliders:
  - S1: Bit crush (MIDI resolution reduction, 0 = full res, 127 = 1-bit)
  - S2: Note stealing (probability to randomly cancel notes, 0-127%)
  - S3: Reverse probability (chance notes play in reverse order)
  - S4: Time stretch (speed variations, 64 = normal, 0 = slow, 127 = fast)

  Creates glitch electronic effects.
]]--

MODE_NAME = "MIDI Mangler"
SLIDER_LABELS = {"Crush", "Steal", "Reverse", "Time"}

-- Note buffer for reverse playback
local note_buffer = {}
local buffer_size = 0
local reverse_mode = false

-- Random seed
local rng_state = 12345

-- Simple LCG random (for deterministic chaos)
function lcg_random()
    rng_state = (rng_state * 1103515245 + 12345) % 2147483648
    return rng_state / 2147483648
end

function init(context)
    print("MIDI Mangler initialized on channel " .. context.midi_channel)
    math.randomseed(os.time())
end

-- Bit crush a value
function bit_crush(value, bits)
    if bits <= 0 then return value end
    local steps = math.floor(128 / (2 ^ bits))
    local crushed = math.floor(value / steps) * steps
    return math.max(0, math.min(127, crushed))
end

function process_event(track, event)
    if not event.switch then
        return
    end

    -- Map S2 to note stealing probability (0-127 -> 0-100%)
    local steal_prob = event.pots[2] / 127.0
    if lcg_random() < steal_prob then
        -- Note stolen! Play nothing
        return
    end

    -- Map S1 to bit crushing (0-127 -> 0-6 bits crushed)
    local crush_amount = math.floor((event.pots[1] / 127.0) * 6)

    -- Bit crush the pitch
    local pitch = event.pots[1]
    pitch = bit_crush(pitch, crush_amount)

    -- Add random pitch jitter from bit crushing
    if crush_amount > 3 then
        local jitter = math.floor(lcg_random() * (crush_amount - 2))
        pitch = pitch + jitter
    end
    pitch = math.max(0, math.min(127, pitch))

    -- Map S3 to reverse probability
    local reverse_prob = event.pots[3] / 127.0
    if lcg_random() < reverse_prob then
        -- Buffer this note for reverse playback
        table.insert(note_buffer, {pitch = pitch, track = track})
        buffer_size = buffer_size + 1

        -- Play buffered notes in reverse every 4 notes
        if buffer_size >= 4 then
            for i = buffer_size, 1, -1 do
                local n = note_buffer[i]
                -- Velocity based on bit crush amount (more crush = quieter)
                local velocity = 100 - (crush_amount * 10)
                velocity = math.max(20, velocity)

                note(n.pitch, velocity, (buffer_size - i) * 30)  -- Stagger timing
                off(n.pitch, 50 + (buffer_size - i) * 30)
            end
            -- Clear buffer
            note_buffer = {}
            buffer_size = 0
        end
        return  -- Don't play the note normally
    end

    -- Map S4 to time stretch (affects note length)
    local time_factor = event.pots[4] / 64.0  -- 64 = normal (1.0x)
    local note_length = math.floor(100 * time_factor)
    note_length = math.max(10, math.min(500, note_length))

    -- Velocity variations based on crush amount
    local velocity = 100 - (crush_amount * 8)
    velocity = math.max(30, velocity)

    -- Random velocity jitter
    velocity = velocity + math.floor((lcg_random() - 0.5) * 20 * (crush_amount / 6.0))
    velocity = math.max(1, math.min(127, velocity))

    -- Play mangled note
    note(pitch, velocity)
    off(pitch, note_length)

    -- Extra glitch: Random re-trigger
    if crush_amount > 4 and lcg_random() < 0.3 then
        local glitch_pitch = pitch + math.floor((lcg_random() - 0.5) * 12)
        glitch_pitch = math.max(0, math.min(127, glitch_pitch))
        note(glitch_pitch, velocity // 2, 50)  -- Delayed glitch
        off(glitch_pitch, 80)
    end
end
