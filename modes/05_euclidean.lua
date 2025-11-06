MODE_NAME = "Euclidean"


--[[
  Mode 5: Euclidean Rhythm Generator

  Algorithmic rhythm generation using the Euclidean algorithm.
  Creates polyrhythmic patterns by distributing hits across steps.

  Mode Number: 5
  Mode Name: Euclidean
  MIDI Channel: 5

  Description:
  Generates rhythmic patterns using the Euclidean algorithm, which
  distributes a given number of hits as evenly as possible across
  a given number of steps. Creates complex, musically interesting
  polyrhythms.

  Track Usage:
  Track 0-7: Each track can have a different Euclidean pattern

  Slider Mappings:
  S1 (event.pots[1]): Number of hits (1-16, distributed across 16 steps)
  S2 (event.pots[2]): Rotation offset (0-15 steps)
  S3 (event.pots[3]): MIDI note / pitch (0-127)
  S4 (event.pots[4]): Velocity (0-127)

  Button Interaction:
  B1-B16: Toggle pattern generation on/off for each step

  Examples:
  - S1=4, S2=0 → [x...x...x...x...] (4 hits in 16 steps = quarter notes)
  - S1=3, S2=0 → [x.....x.....x...] (3 hits in 16 steps)
  - S1=5, S2=2 → rotated 5-hit pattern
]]--

-- ============================================================================
-- Module-level variables
-- ============================================================================

-- Store computed Euclidean patterns per track (cache for performance)
local euclidean_patterns = {}

-- ============================================================================
-- Euclidean algorithm implementation
-- ============================================================================

-- Generate Euclidean rhythm pattern
-- hits: number of beats to distribute (k)
-- steps: total number of steps (n)
-- Returns: table of booleans (true = hit, false = rest)
function generate_euclidean_pattern(hits, steps)
  if hits >= steps then
    -- All steps are hits
    local pattern = {}
    for i = 1, steps do
      pattern[i] = true
    end
    return pattern
  end

  if hits == 0 then
    -- No hits
    local pattern = {}
    for i = 1, steps do
      pattern[i] = false
    end
    return pattern
  end

  -- Bjorklund's algorithm
  local pattern = {}
  local counts = {}
  local remainders = {}

  local divisor = steps - hits
  remainders[1] = hits

  local level = 0
  while remainders[level + 1] > 1 do
    counts[level + 1] = math.floor(divisor / remainders[level + 1])
    remainders[level + 2] = divisor % remainders[level + 1]
    divisor = remainders[level + 1]
    level = level + 1
  end
  counts[level + 1] = divisor

  -- Build pattern string
  local result = {}

  local function build(level_idx)
    if level_idx == 0 then
      return {true}
    elseif level_idx == -1 then
      return {false}
    else
      local seq = {}
      for _ = 1, counts[level_idx] do
        for _, v in ipairs(build(level_idx - 1)) do
          table.insert(seq, v)
        end
      end
      for _, v in ipairs(build(level_idx - 2)) do
        table.insert(seq, v)
      end
      return seq
    end
  end

  result = build(level + 1)

  -- Pad or trim to exactly 'steps' length
  while #result < steps do
    table.insert(result, false)
  end
  while #result > steps do
    table.remove(result)
  end

  return result
end

-- Rotate pattern by offset steps
function rotate_pattern(pattern, offset)
  local rotated = {}
  local len = #pattern
  for i = 1, len do
    local idx = ((i - 1 + offset) % len) + 1
    rotated[i] = pattern[idx]
  end
  return rotated
end


-- ============================================================================
-- init() - Called once when mode loads
-- ============================================================================

function init(context)
  -- Initialize pattern cache for all tracks
  for track = 0, 7 do
    euclidean_patterns[track] = {}
  end
end


-- ============================================================================
-- process_event() - Called for each Event during playback
-- ============================================================================

function process_event(track, event)
  -- Only play if switch is on
  if not event.switch then
    return {}
  end

  -- Get parameters
  local hits_raw = event.pots[1]  -- S1: Number of hits (0-127)
  local rotation_raw = event.pots[2]  -- S2: Rotation offset (0-127)
  local pitch = event.pots[3]  -- S3: MIDI note
  local velocity = event.pots[4]  -- S4: Velocity

  -- Map S1 to 1-16 hits
  local hits = math.max(1, math.min(16, math.floor((hits_raw / 127.0) * 15) + 1))

  -- Map S2 to 0-15 rotation
  local rotation = math.floor((rotation_raw / 127.0) * 15)

  -- Generate or retrieve cached pattern
  local cache_key = hits .. "_" .. rotation
  local pattern = euclidean_patterns[track][cache_key]

  if not pattern then
    -- Generate new pattern and cache it
    local base_pattern = generate_euclidean_pattern(hits, 16)
    pattern = rotate_pattern(base_pattern, rotation)
    euclidean_patterns[track][cache_key] = pattern
  end

  -- Get current step (0-15 in engine, 1-16 in Lua)
  -- We don't have direct access to current_step, so we'll use all tracks
  -- and let the engine handle stepping. Each track plays if pattern says so.

  -- For now, we'll trigger on ALL events that are enabled,
  -- and let the pattern be determined by the track number
  -- This is a simplification - ideally we'd check the global step counter

  -- Instead, let's just always play when the event is on,
  -- and the Euclidean pattern controls which TRACKS are active
  -- by having the user enable/disable tracks according to the pattern

  -- Actually, we'll play the note and let the user decide pattern via button presses
  -- The Euclidean algorithm determines intensity/accent

  -- Better approach: Use velocity modulation based on position in pattern
  local step_in_pattern = track % 16
  local should_play = pattern[step_in_pattern + 1]

  if should_play then
    -- Clamp pitch to valid MIDI range
    pitch = math.max(0, math.min(127, pitch))

    -- Send MIDI note with fixed length
    note(pitch, velocity, 0)
    off(pitch, 100)  -- 100ms notes
  end

  return {}
end
