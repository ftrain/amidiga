--[[
  Mode 9: Cellular Automaton Sequencer

  Conway's Game of Life controls MIDI notes. The 8x16 grid of events
  becomes a cellular automaton where alive cells = notes play.
  Patterns evolve over time creating emergent rhythms.

  Sliders:
  - S1: Survival rule (how many neighbors to survive, 0-127 -> 2-4)
  - S2: Birth rule (how many neighbors to birth, 0-127 -> 2-4)
  - S3: Base pitch offset (for generated notes)
  - S4: Velocity (for all notes)

  Tracks represent rows, steps represent columns.
]]--

MODE_NAME = "Cellular Automaton"

-- Grid state: 8 tracks x 16 steps (true = alive)
local grid = {}
local next_grid = {}

-- Initialize grids
for t = 0, 7 do
    grid[t] = {}
    next_grid[t] = {}
    for s = 0, 15 do
        grid[t][s] = false
        next_grid[t][s] = false
    end
end

-- Step counter to know when to evolve
local current_step = 0
local last_step = -1

function init(context)
    print("Cellular Automaton initialized on channel " .. context.midi_channel)
end

-- Count live neighbors (8-connected)
function count_neighbors(track, step)
    local count = 0
    for dt = -1, 1 do
        for ds = -1, 1 do
            if not (dt == 0 and ds == 0) then  -- Don't count self
                local t = (track + dt) % 8
                local s = (step + ds) % 16
                if grid[t][s] then
                    count = count + 1
                end
            end
        end
    end
    return count
end

-- Evolve the grid (apply Game of Life rules)
function evolve_grid(survival_threshold, birth_threshold)
    -- Calculate next generation
    for t = 0, 7 do
        for s = 0, 15 do
            local neighbors = count_neighbors(t, s)
            local alive = grid[t][s]

            if alive then
                -- Survival rule: stay alive if neighbors in range
                next_grid[t][s] = (neighbors >= survival_threshold and neighbors <= survival_threshold + 1)
            else
                -- Birth rule: become alive if exactly birth_threshold neighbors
                next_grid[t][s] = (neighbors >= birth_threshold and neighbors <= birth_threshold + 1)
            end
        end
    end

    -- Swap grids
    for t = 0, 7 do
        for s = 0, 15 do
            grid[t][s] = next_grid[t][s]
        end
    end
end

function process_event(track, event)
    -- Track current step (0-15)
    local step = current_step % 16

    -- Seed the grid with programmed events on first pass
    if event.switch and not grid[track][step] then
        grid[track][step] = true
    end

    -- Evolve grid at step 0 (once per bar)
    if step == 0 and current_step ~= last_step then
        -- Map S1 to survival rule (0-127 -> 2-4)
        local survival = 2 + math.floor((event.pots[1] * 2) / 127)
        -- Map S2 to birth rule (0-127 -> 2-4)
        local birth = 2 + math.floor((event.pots[2] * 2) / 127)

        evolve_grid(survival, birth)
        last_step = current_step
    end

    -- Play note if cell is alive
    if grid[track][step] then
        -- Base pitch from S3, offset by track number
        local base_pitch = event.pots[3]
        local pitch = base_pitch + track * 2  -- Each track 2 semitones apart

        -- Velocity from S4
        local velocity = event.pots[4]
        if velocity < 20 then velocity = 80 end  -- Default if not set

        note(pitch, velocity)
        off(pitch, 100)
    end

    current_step = current_step + 1
end
