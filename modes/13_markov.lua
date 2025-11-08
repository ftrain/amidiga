--[[
  Mode 13: Markov Chain Melody Generator

  AI-inspired probabilistic melody generator. Learns from programmed
  patterns and generates new melodies based on note transition probabilities.

  Sliders:
  - S1: Memory (how much to remember previous notes, 0 = amnesia, 127 = elephant)
  - S2: Creativity (randomness, 0 = conservative, 127 = wild improvisation)
  - S3: Scale constraint (0 = chromatic, 64 = pentatonic, 127 = blues)
  - S4: Note length variance (0 = uniform, 127 = highly varied)

  Each track maintains its own Markov chain state.
]]--

MODE_NAME = "Markov Chain"
SLIDER_LABELS = {"Memory", "Creative", "Scale", "Variance"}

-- Transition table: [from_note][to_note] = count
local transitions = {}

-- Current note for each track
local current_note = {60, 62, 64, 65, 67, 69, 71, 72}

-- Note history for learning
local note_history = {}

-- Initialize transition table
for i = 0, 127 do
    transitions[i] = {}
    for j = 0, 127 do
        transitions[i][j] = 0
    end
end

-- Scale definitions
local pentatonic = {0, 2, 4, 7, 9}
local blues = {0, 3, 5, 6, 7, 10}

function init(context)
    print("Markov Chain initialized on channel " .. context.midi_channel)
    math.randomseed(os.time())
end

-- Quantize to scale
function quantize_to_scale(note, scale_mode)
    if scale_mode < 43 then
        return note  -- Chromatic (no quantization)
    end

    local scale = (scale_mode < 95) and pentatonic or blues
    local octave = math.floor(note / 12)
    local pitch_class = note % 12

    -- Find closest scale note
    local closest = scale[1]
    local min_dist = 12
    for _, interval in ipairs(scale) do
        local dist = math.abs(pitch_class - interval)
        if dist < min_dist then
            min_dist = dist
            closest = interval
        end
    end

    return octave * 12 + closest
end

-- Learn from programmed pattern
function learn_transition(from_note, to_note, memory_amount)
    if from_note and to_note then
        -- Increase transition count (weighted by memory)
        local weight = 1 + math.floor(memory_amount * 3)
        transitions[from_note][to_note] = transitions[from_note][to_note] + weight
    end
end

-- Choose next note based on Markov chain
function generate_next_note(from_note, creativity)
    local total_weight = 0
    local candidates = {}

    -- Gather all possible next notes with weights
    for to_note = 0, 127 do
        local weight = transitions[from_note][to_note]
        if weight > 0 then
            total_weight = total_weight + weight
            table.insert(candidates, {note = to_note, weight = weight})
        end
    end

    -- If no learned transitions, use random walk
    if total_weight == 0 then
        local step = math.floor((math.random() - 0.5) * 12 * creativity)
        return math.max(0, math.min(127, from_note + step))
    end

    -- Apply creativity: blend learned transitions with randomness
    local random_factor = creativity
    local use_random = (math.random() < random_factor)

    if use_random then
        -- Creative mode: random walk from current note
        local step = math.floor((math.random() - 0.5) * 24 * creativity)
        return math.max(0, math.min(127, from_note + step))
    else
        -- Conservative mode: weighted choice from learned transitions
        local r = math.random() * total_weight
        local cumsum = 0
        for _, cand in ipairs(candidates) do
            cumsum = cumsum + cand.weight
            if r <= cumsum then
                return cand.note
            end
        end
        return candidates[#candidates].note
    end
end

function process_event(track, event)
    if not event.switch then
        return
    end

    -- Map S1 to memory (0-127 -> 0.0 to 1.0)
    local memory = event.pots[1] / 127.0

    -- Map S2 to creativity (0-127 -> 0.0 to 1.0)
    local creativity = event.pots[2] / 127.0

    -- Learn from this programmed event
    local programmed_pitch = event.pots[1]
    table.insert(note_history, programmed_pitch)

    -- Learn transitions from history
    if #note_history >= 2 then
        local from = note_history[#note_history - 1]
        local to = note_history[#note_history]
        learn_transition(from, to, memory)
    end

    -- Generate next note using Markov chain
    local next_note = generate_next_note(current_note[track + 1], creativity)

    -- Apply scale quantization (S3)
    next_note = quantize_to_scale(next_note, event.pots[3])
    next_note = math.max(0, math.min(127, next_note))

    -- Update current note for this track
    current_note[track + 1] = next_note

    -- Map S4 to note length variance
    local variance = event.pots[4] / 127.0
    local base_length = 100
    local length_variation = math.floor((math.random() - 0.5) * 150 * variance)
    local note_length = base_length + length_variation
    note_length = math.max(30, math.min(300, note_length))

    -- Velocity varies with creativity (more creative = more dynamic)
    local velocity = 80 + math.floor((math.random() - 0.5) * 40 * creativity)
    velocity = math.max(40, math.min(120, velocity))

    -- Play generated note
    note(next_note, velocity)
    off(next_note, note_length)
end
