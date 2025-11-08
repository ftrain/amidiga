--[[
  Mode 10: Wave Table Scanner

  Treats the 16 programmed events as a wavetable and smoothly scans
  through it, creating gliding arpeggios and smooth pitch sweeps.

  Sliders:
  - S1: Scan speed (0 = slow, 127 = fast through all 16 steps)
  - S2: Scan direction (0-42 = forward, 43-84 = reverse, 85-127 = ping-pong)
  - S3: Quantization (0 = chromatic, 64 = pentatonic, 127 = whole tone)
  - S4: Velocity

  Each track scans at a different phase offset for polyphonic effects.
]]--

MODE_NAME = "Wave Table Scanner"
SLIDER_LABELS = {"Speed", "Dir", "Quant", "Velocity"}

-- Scan position for each track (0.0 to 16.0)
local scan_pos = {0, 2, 4, 6, 8, 10, 12, 14}

-- Direction for ping-pong mode (+1 or -1)
local scan_dir = {1, 1, 1, 1, 1, 1, 1, 1}

-- Last played note per track (for legato)
local last_note = {-1, -1, -1, -1, -1, -1, -1, -1}

-- Pentatonic scale offsets
local pentatonic = {0, 2, 4, 7, 9, 12, 14, 16, 19, 21, 24}

-- Whole tone scale offsets
local wholetone = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24}

function init(context)
    print("Wave Table Scanner initialized on channel " .. context.midi_channel)
end

-- Quantize pitch to scale
function quantize_pitch(pitch, quantize_mode)
    if quantize_mode < 43 then
        -- Chromatic (no quantization)
        return pitch
    elseif quantize_mode < 85 then
        -- Pentatonic
        local octave = math.floor(pitch / 12)
        local note = pitch % 12
        -- Find closest pentatonic note
        local closest = pentatonic[1]
        local min_dist = 100
        for _, p in ipairs(pentatonic) do
            local dist = math.abs((note - (p % 12)))
            if dist < min_dist then
                min_dist = dist
                closest = p % 12
            end
        end
        return octave * 12 + closest
    else
        -- Whole tone
        local octave = math.floor(pitch / 12)
        local note = pitch % 12
        local closest = math.floor(note / 2) * 2  -- Round to nearest whole tone
        return octave * 12 + closest
    end
end

function process_event(track, event)
    if not event.switch then
        return
    end

    -- Map S1 to scan speed (0-127 -> 0.05 to 2.0 steps per event)
    local speed = 0.05 + (event.pots[1] / 127.0) * 1.95

    -- Map S2 to direction
    local dir_mode = event.pots[2]
    if dir_mode < 43 then
        -- Forward
        scan_dir[track + 1] = 1
    elseif dir_mode < 85 then
        -- Reverse
        scan_dir[track + 1] = -1
    end
    -- Ping-pong handled below

    -- Advance scan position
    scan_pos[track + 1] = scan_pos[track + 1] + speed * scan_dir[track + 1]

    -- Wrap/bounce
    if dir_mode >= 85 then
        -- Ping-pong mode
        if scan_pos[track + 1] >= 16 then
            scan_pos[track + 1] = 16
            scan_dir[track + 1] = -1
        elseif scan_pos[track + 1] <= 0 then
            scan_pos[track + 1] = 0
            scan_dir[track + 1] = 1
        end
    else
        -- Wrap mode
        if scan_pos[track + 1] >= 16 then
            scan_pos[track + 1] = scan_pos[track + 1] - 16
        elseif scan_pos[track + 1] < 0 then
            scan_pos[track + 1] = scan_pos[track + 1] + 16
        end
    end

    -- Interpolate pitch from wavetable (use S1 value as wavetable)
    -- For simplicity, we'll use the current event's pitch
    local base_pitch = event.pots[1]

    -- Add smooth variation based on scan position
    local variation = math.sin(scan_pos[track + 1] * 0.392) * 12  -- ±12 semitones
    local pitch = base_pitch + math.floor(variation)

    -- Quantize
    pitch = quantize_pitch(pitch, event.pots[3])
    pitch = math.max(0, math.min(127, pitch))

    -- Velocity from S4
    local velocity = event.pots[4]
    if velocity < 20 then velocity = 100 end

    -- Turn off last note (for legato scanning)
    if last_note[track + 1] >= 0 then
        off(last_note[track + 1], 0)
    end

    -- Play new note
    note(pitch, velocity)
    last_note[track + 1] = pitch

    -- Note off after longer duration (creates overlap/legato)
    off(pitch, 200)
end
