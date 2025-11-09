#pragma once

#include <cstdint>

namespace gruvbok {
namespace config {

// ============================================================================
// Timing Constants
// ============================================================================

/// Autosave interval (milliseconds)
/// Used by Engine to periodically save song data
constexpr uint32_t AUTOSAVE_INTERVAL_MS = 20000;

/// LED tempo indicator duration (milliseconds)
/// How long the LED stays on for each beat
constexpr uint32_t LED_TEMPO_DURATION_MS = 50;

/// Tempo change debounce delay (milliseconds)
/// Wait time after last tempo change before autosaving
constexpr uint32_t TEMPO_DEBOUNCE_MS = 1000;

// ============================================================================
// Musical Constants
// ============================================================================

/// Minimum tempo (BPM)
/// Lower bound for tempo adjustment
constexpr int TEMPO_MIN_BPM = 60;

/// Maximum tempo (BPM)
/// Upper bound for tempo adjustment
constexpr int TEMPO_MAX_BPM = 240;

/// Default tempo (BPM)
/// Initial tempo on startup
constexpr int TEMPO_DEFAULT_BPM = 120;

/// MIDI clock pulses per quarter note (MIDI standard)
/// Used for MIDI clock synchronization (24 PPQN is MIDI spec)
constexpr int MIDI_PPQN = 24;

/// Steps per bar (16th note resolution)
/// Number of steps in one measure at 16th note resolution
constexpr int STEPS_PER_BAR = 16;

/// Sixteenth notes per quarter note
/// Used for step interval calculation
constexpr int DIVISIONS_PER_QUARTER = 4;

/// Milliseconds per minute
/// Constant for BPM to ms conversion
constexpr int MS_PER_MINUTE = 60000;

// ============================================================================
// Hardware Constants
// ============================================================================

/// Number of button inputs (B1-B16)
/// Matches the 16 steps in a bar
constexpr int NUM_BUTTONS = 16;

/// Number of rotary pots (R1-R4)
/// Mode, Tempo, Pattern, Track selectors
constexpr int NUM_ROTARY_POTS = 4;

/// Number of slider pots (S1-S4)
/// Mode-specific parameters
constexpr int NUM_SLIDER_POTS = 4;

/// MIDI value range maximum
/// Standard MIDI 7-bit value range (0-127)
constexpr int MIDI_MAX_VALUE = 127;

/// MIDI value range minimum
constexpr int MIDI_MIN_VALUE = 0;

/// LED brightness range maximum (PWM)
/// 8-bit PWM range for LED brightness
constexpr int LED_BRIGHTNESS_MAX = 255;

// ============================================================================
// Data Structure Sizes
// ============================================================================

/// Events per track (matches button count)
/// Each button corresponds to one event in the track
constexpr int EVENTS_PER_TRACK = 16;

/// Tracks per pattern
/// Allows for multi-track composition within a pattern
constexpr int TRACKS_PER_PATTERN = 8;

/// Patterns per mode
/// Provides sufficient variation per mode
constexpr int PATTERNS_PER_MODE = 32;

/// Total modes in song
/// One mode per MIDI channel (0-14, channel 15 reserved)
constexpr int NUM_MODES = 15;

/// Song mode loop length (bars)
/// Default number of bars for song mode pattern sequencing
constexpr int SONG_MODE_DEFAULT_LOOP_LENGTH = 16;

// ============================================================================
// Rotary Pot Assignments
// ============================================================================

/// R1: Mode selector (0-14)
constexpr int POT_MODE = 0;

/// R2: Tempo selector (60-240 BPM)
constexpr int POT_TEMPO = 1;

/// R3: Pattern selector (0-31)
constexpr int POT_PATTERN = 2;

/// R4: Track selector (0-7)
constexpr int POT_TRACK = 3;

// ============================================================================
// Slider Pot Assignments (Mode-Specific)
// ============================================================================

/// S1: Mode-specific parameter 1
constexpr int SLIDER_PARAM_1 = 0;

/// S2: Mode-specific parameter 2
constexpr int SLIDER_PARAM_2 = 1;

/// S3: Mode-specific parameter 3
constexpr int SLIDER_PARAM_3 = 2;

/// S4: Mode-specific parameter 4
constexpr int SLIDER_PARAM_4 = 3;

// ============================================================================
// LED Pattern Timing (milliseconds)
// ============================================================================

/// Fast double-blink: on duration (first pulse)
constexpr uint32_t LED_FAST_BLINK_ON1_MS = 100;

/// Fast double-blink: off duration (between pulses)
constexpr uint32_t LED_FAST_BLINK_OFF_MS = 50;

/// Fast double-blink: on duration (second pulse)
constexpr uint32_t LED_FAST_BLINK_ON2_MS = 100;

/// Fast double-blink: pause duration (between cycles)
constexpr uint32_t LED_FAST_BLINK_PAUSE_MS = 150;

/// Total cycle time for fast double-blink pattern
constexpr uint32_t LED_FAST_BLINK_CYCLE_MS =
    LED_FAST_BLINK_ON1_MS +
    LED_FAST_BLINK_OFF_MS +
    LED_FAST_BLINK_ON2_MS +
    LED_FAST_BLINK_PAUSE_MS;

// ============================================================================
// Bit-Packing Constants (Event structure)
// ============================================================================

/// Switch bit position in packed Event
constexpr int EVENT_SWITCH_SHIFT = 0;

/// First pot (S1) bit position
constexpr int EVENT_POT0_SHIFT = 1;

/// Second pot (S2) bit position
constexpr int EVENT_POT1_SHIFT = 8;

/// Third pot (S3) bit position
constexpr int EVENT_POT2_SHIFT = 15;

/// Fourth pot (S4) bit position
constexpr int EVENT_POT3_SHIFT = 22;

/// Pot value bit mask (7 bits for MIDI value)
constexpr uint32_t EVENT_POT_MASK = 0x7F;

/// Switch bit mask
constexpr uint32_t EVENT_SWITCH_MASK = 0x00000001;

// ============================================================================
// Memory Constraints
// ============================================================================

/// Total events in song data structure
/// 15 modes × 32 patterns × 8 tracks × 16 events = 61,440 events
constexpr int TOTAL_EVENTS = NUM_MODES * PATTERNS_PER_MODE * TRACKS_PER_PATTERN * EVENTS_PER_TRACK;

/// Bytes per event (bit-packed into uint32_t)
constexpr int BYTES_PER_EVENT = 4;

/// Total memory for event data (bytes)
/// Used for Teensy memory planning: ~245 KB
constexpr int EVENT_DATA_SIZE_BYTES = TOTAL_EVENTS * BYTES_PER_EVENT;

} // namespace config
} // namespace gruvbok
