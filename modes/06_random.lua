MODE_NAME = "Random"


--[[
  Mode 6: Random/Generative

  Probabilistic note generation with controllable randomness.
  Creates evolving, generative patterns that never repeat exactly.

  Mode Number: 6
  Mode Name: Random
  MIDI Channel: 6

  Description:
  Generates notes based on probability and random ranges. Each step
  has a chance to trigger, and the pitch/velocity are randomly
  selected within user-defined ranges. Great for ambient textures,
  evolving basslines, and experimental sequences.

  Track Usage:
  Track 0-7: Each track is an independent random voice

  Slider Mappings:
  S1 (event.pots[1]): Trigger probability (0-127, 0=never, 127=always)
  S2 (event.pots[2]): Pitch center (0-127, MIDI note number)
  S3 (event.pots[3]): Pitch range (0-127, ±semitones from center)
  S4 (event.pots[4]): Velocity range (0-127, average velocity)

  Button Interaction:
  B1-B16: Toggle random generation on/off for each step

  Examples:
  - S1=127, S2=60, S3=0, S4=100 → Always plays C4 at velocity 100
  - S1=64, S2=60, S3=12, S4=80 → 50% chance, C4 ±1 octave, ~velocity 80
  - S1=32, S2=36, S3=24, S4=60 → Sparse, low random notes
]]--

-- ============================================================================
-- Module-level variables
-- ============================================================================

-- Seed random number generator with current time (called in init)
local rng_seed = 0


-- ============================================================================
-- Helper functions
-- ============================================================================

-- Simple pseudo-random number generator (Linear Congruential Generator)
-- We use this instead of math.random() for deterministic behavior
local rng_state = 12345

function random_int(min_val, max_val)
  -- LCG: X(n+1) = (a * X(n) + c) mod m
  rng_state = (1103515245 * rng_state + 12345) % 2147483648
  local range = max_val - min_val + 1
  return min_val + (rng_state % range)
end

function random_float()
  rng_state = (1103515245 * rng_state + 12345) % 2147483648
  return rng_state / 2147483648.0
end

-- Map value from one range to another
function map_range(value, in_min, in_max, out_min, out_max)
  return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min
end

-- Clamp value to range
function clamp(value, min_val, max_val)
  return math.max(min_val, math.min(max_val, value))
end


-- ============================================================================
-- init() - Called once when mode loads
-- ============================================================================

function init(context)
  -- Seed RNG with something pseudo-random
  -- We use the mode number and tempo to create a seed
  rng_seed = context.mode_number * 1000 + context.tempo
  rng_state = rng_seed
end


-- ============================================================================
-- process_event() - Called for each Event during playback
-- ============================================================================

function process_event(track, event)
  -- Only consider if switch is on
  if not event.switch then
    return {}
  end

  -- Get parameters
  local probability_raw = event.pots[1]  -- S1: Trigger probability (0-127)
  local pitch_center = event.pots[2]  -- S2: Pitch center (0-127)
  local pitch_range_raw = event.pots[3]  -- S3: Pitch range (0-127)
  local velocity_avg = event.pots[4]  -- S4: Velocity average (0-127)

  -- Map probability to 0-100%
  local probability = probability_raw / 127.0

  -- Check if we should trigger this note (probabilistic)
  local trigger_roll = random_float()
  if trigger_roll > probability then
    -- Don't trigger this time
    return {}
  end

  -- Map pitch range to ±semitones (0-127 -> 0-24 semitones = 2 octaves)
  local pitch_range = math.floor((pitch_range_raw / 127.0) * 24)

  -- Generate random pitch within range
  local pitch_offset = random_int(-pitch_range, pitch_range)
  local pitch = pitch_center + pitch_offset

  -- Clamp to valid MIDI range
  pitch = clamp(pitch, 0, 127)

  -- Generate random velocity around average (±20%)
  local velocity_variation = math.floor(velocity_avg * 0.2)
  local velocity = random_int(
    math.max(1, velocity_avg - velocity_variation),
    math.min(127, velocity_avg + velocity_variation)
  )

  -- Random note length (50-200ms for variety)
  local note_length = random_int(50, 200)

  -- Send MIDI note
  note(pitch, velocity, 0)
  off(pitch, note_length)

  -- Randomly send filter modulation CC for evolving texture
  if random_float() > 0.7 then
    local filter_value = random_int(40, 100)
    cc(74, filter_value, 0)  -- CC 74 = Filter cutoff
  end

  return {}
end
