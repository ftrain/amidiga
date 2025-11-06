--[[
  Mode 7: Sample & Hold / Glitch

  Sample and hold algorithm creates stepped, glitchy sequences by
  randomly sampling and holding parameter values.

  Mode Number: 7
  Mode Name: Sample & Hold
  MIDI Channel: 7

  Description:
  Creates glitchy, stepped sequences by sampling random values and
  holding them for a variable duration. Values change suddenly rather
  than continuously, creating a classic sample & hold effect. Great
  for experimental, glitchy, IDM-style sequences.

  Track Usage:
  Track 0-7: Each track is an independent sample & hold voice

  Slider Mappings:
  S1 (event.pots[1]): Sample rate (0-127, how often to resample)
  S2 (event.pots[2]): Pitch quantization (0-127, maps to scale)
  S3 (event.pots[3]): Glitch density (0-127, chance of double-trigger)
  S4 (event.pots[4]): Modulation depth (0-127, filter/mod wheel amount)

  Button Interaction:
  B1-B16: Toggle sample & hold on/off for each step

  Technical:
  - Uses chromatic scale with sample & hold on pitch
  - Occasional double-triggers for glitch effect
  - Random CC modulation for evolving timbre
]]--

-- ============================================================================
-- Module-level variables
-- ============================================================================

-- Per-track sample & hold state
local sh_state = {}

-- Chromatic scale starting from C2
local scale_notes = {36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,  -- C2-B2
                     48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,  -- C3-B3
                     60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71}  -- C4-B4

-- Simple LCG random number generator
local rng_state = 42

function sh_random()
  rng_state = (1103515245 * rng_state + 12345) % 2147483648
  return rng_state / 2147483648.0
end

function sh_random_int(min_val, max_val)
  local range = max_val - min_val + 1
  return min_val + math.floor(sh_random() * range)
end


-- ============================================================================
-- init() - Called once when mode loads
-- ============================================================================

function init(context)
  -- Initialize sample & hold state for each track
  for track = 0, 7 do
    sh_state[track] = {
      held_pitch = 60,        -- Currently held pitch
      held_velocity = 80,     -- Currently held velocity
      held_filter = 64,       -- Currently held filter value
      sample_counter = 0,     -- Counter for sample rate
      glitch_active = false   -- Glitch state
    }
  end

  -- Seed RNG
  rng_state = context.tempo * 100 + context.mode_number
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
  local sample_rate = event.pots[1]  -- S1: Sample rate (0-127)
  local pitch_quant = event.pots[2]  -- S2: Pitch quantization
  local glitch_density = event.pots[3]  -- S3: Glitch density
  local mod_depth = event.pots[4]  -- S4: Modulation depth

  -- Initialize state for this track if needed
  if not sh_state[track] then
    sh_state[track] = {
      held_pitch = 60,
      held_velocity = 80,
      held_filter = 64,
      sample_counter = 0,
      glitch_active = false
    }
  end

  local state = sh_state[track]

  -- Increment sample counter
  state.sample_counter = state.sample_counter + 1

  -- Map sample rate to resample frequency (1-16 steps)
  -- Low sample_rate = frequent resampling, high = hold longer
  local resample_freq = 1 + math.floor((sample_rate / 127.0) * 15)

  -- Check if we should resample
  if state.sample_counter >= resample_freq then
    state.sample_counter = 0

    -- Sample new random pitch from quantized scale
    local scale_index = sh_random_int(1, #scale_notes)
    state.held_pitch = scale_notes[scale_index]

    -- Sample new velocity (60-120 range for variation)
    state.held_velocity = sh_random_int(60, 120)

    -- Sample new filter value
    state.held_filter = sh_random_int(30, 127)
  end

  -- Use held values
  local pitch = state.held_pitch
  local velocity = state.held_velocity

  -- Glitch effect: occasional double-trigger
  local glitch_chance = glitch_density / 127.0
  local is_glitch = sh_random() < glitch_chance

  if is_glitch then
    -- Double trigger with slight pitch offset
    note(pitch, velocity, 0)
    off(pitch, 30)
    note(pitch + 1, velocity - 10, 40)  -- Slightly offset
    off(pitch + 1, 30)
  else
    -- Normal trigger
    note(pitch, velocity, 0)
    off(pitch, 80)
  end

  -- Send filter CC based on held value and modulation depth
  local filter_amount = math.floor((mod_depth / 127.0) * state.held_filter)
  cc(74, filter_amount, 0)  -- CC 74 = Filter cutoff

  -- Occasional modulation wheel movement for evolving sound
  if sh_random() > 0.8 then
    local mod_wheel = sh_random_int(0, 100)
    cc(1, mod_wheel, 0)  -- CC 1 = Modulation wheel
  end

  return {}
end
