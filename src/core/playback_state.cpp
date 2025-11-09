#include "playback_state.h"
#include "pattern.h"  // For Pattern::NUM_TRACKS

namespace gruvbok {

PlaybackState::PlaybackState(HardwareInterface* hardware)
    : hardware_(hardware)
    , is_playing_(false)
    , tempo_(120)
    , current_mode_(1)  // Start with mode 1 (drums)
    , current_pattern_(0)
    , current_track_(0)
    , current_step_(0)
    , target_mode_(1)  // Default target mode for Mode 0 editing
    , last_step_time_(0)
    , step_interval_ms_(0)
    , lua_reinit_pending_(false)
    , last_tempo_change_time_(0) {
    calculateStepInterval();
}

void PlaybackState::start() {
    is_playing_ = true;
    current_step_ = 0;
    last_step_time_ = hardware_->getMillis();
}

void PlaybackState::stop() {
    is_playing_ = false;
}

bool PlaybackState::shouldAdvanceStep(uint32_t current_time) const {
    if (!is_playing_) {
        return false;
    }
    return (current_time - last_step_time_) >= step_interval_ms_;
}

void PlaybackState::advanceStep(uint32_t current_time) {
    last_step_time_ = current_time;

    // Advance step with bounds checking (0-15)
    current_step_ = (current_step_ + 1) % Track::NUM_EVENTS;  // NUM_EVENTS = 16
}

void PlaybackState::setTempo(int bpm) {
    // Clamp tempo to valid range (1-1000 BPM)
    tempo_ = std::clamp(bpm, 1, 1000);
    calculateStepInterval();

    // Mark Lua reinit as pending (debounced)
    lua_reinit_pending_ = true;
    last_tempo_change_time_ = hardware_->getMillis();
}

void PlaybackState::setMode(int mode) {
    // Bounds check: mode must be 0-14 (NUM_MODES = 15)
    if (mode >= 0 && mode < Song::NUM_MODES) {
        current_mode_ = mode;
    }
}

void PlaybackState::setPattern(int pattern) {
    // Bounds check: pattern must be 0-31 (NUM_PATTERNS = 32)
    if (pattern >= 0 && pattern < Mode::NUM_PATTERNS) {
        current_pattern_ = pattern;
    }
}

void PlaybackState::setTrack(int track) {
    // Bounds check: track must be 0-7 (NUM_TRACKS = 8)
    if (track >= 0 && track < Pattern::NUM_TRACKS) {
        current_track_ = track;
    }
}

void PlaybackState::setTargetMode(int mode) {
    // Bounds check: target_mode must be 1-14 (not mode 0)
    if (mode >= 1 && mode < Song::NUM_MODES) {
        target_mode_ = mode;
    }
}

bool PlaybackState::isLuaReinitPending(uint32_t current_time) const {
    if (!lua_reinit_pending_) {
        return false;
    }
    return (current_time - last_tempo_change_time_) >= TEMPO_DEBOUNCE_MS;
}

void PlaybackState::clearLuaReinitPending() {
    lua_reinit_pending_ = false;
}

void PlaybackState::calculateStepInterval() {
    // Calculate time per step in milliseconds
    // At 120 BPM: 1 beat = 500ms, 16 steps per bar = 4 beats, so 1 step = 125ms
    // Formula: (60000 / BPM) / 4 = ms per step (assuming 16th notes)
    step_interval_ms_ = (60000 / tempo_) / 4;
}

} // namespace gruvbok
