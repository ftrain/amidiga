#include "mode0_sequencer.h"
#include <iostream>
#include <algorithm>

namespace gruvbok {

Mode0Sequencer::Mode0Sequencer(Song* song)
    : song_(song)
    , song_mode_step_(0)
    , song_mode_loop_length_(16)  // Default to full 16 steps
    , global_scale_root_(0)  // C
    , global_scale_type_(0) { // Ionian/Major

    // Initialize per-mode arrays
    for (int i = 0; i < Song::NUM_MODES; ++i) {
        mode_velocity_offsets_[i] = 0;
        mode_pattern_overrides_[i] = -1;  // -1 means no override
    }
}

void Mode0Sequencer::start() {
    song_mode_step_ = 0;
}

void Mode0Sequencer::advanceStep() {
    // Advance with bounds checking
    song_mode_step_ = (song_mode_step_ + 1) % song_mode_loop_length_;

    std::cout << "Mode 0 step advanced to: " << song_mode_step_
              << " (loop length: " << song_mode_loop_length_ << ")" << std::endl;
}

void Mode0Sequencer::calculateLoopLength() {
    // Scan Mode 0, Pattern 0, Track 0 only (Mode 0 uses only Track 0)
    // Find the highest step number with switch on
    Mode& mode0 = song_->getMode(0);
    Pattern& pattern = mode0.getPattern(0);

    int max_step = -1;  // Start at -1 so first active step sets it
    for (int step = 0; step < Track::NUM_EVENTS; ++step) {  // NUM_EVENTS = 16
        const Event& event = pattern.getEvent(0, step);  // Track 0 only
        if (event.getSwitch()) {
            max_step = step;  // Keep updating to find the highest active step
        }
    }

    // Loop length is max_step + 1 (e.g., if B4 is pressed, max_step=3, loop_length=4)
    // IMPORTANT: Clamp to valid range [1, 16] to prevent crashes
    song_mode_loop_length_ = std::clamp(max_step + 1, 1, Track::NUM_EVENTS);

    std::cout << "[Mode 0] Loop length: " << song_mode_loop_length_
              << " steps (last active: " << max_step << ")" << std::endl;
}

void Mode0Sequencer::applyParameters() {
    // Read Mode 0 Track 0 event at the current song_mode_step_
    // Mode 0 uses only Track 0 to set pattern for ALL modes simultaneously
    Mode& mode0 = song_->getMode(0);
    Pattern& pattern = mode0.getPattern(0);

    // Bounds check: ensure song_mode_step_ is valid
    if (song_mode_step_ < 0 || song_mode_step_ >= Track::NUM_EVENTS) {
        std::cerr << "[Mode0Sequencer] Invalid song_mode_step_: " << song_mode_step_ << std::endl;
        return;
    }

    // Get event for current song mode step (Track 0 only)
    const Event& event = pattern.getEvent(0, song_mode_step_);

    // If this step is active, apply pattern to all modes 1-14
    if (event.getSwitch()) {
        // S1: Pattern (0-127 maps to 0-31)
        uint8_t s1 = event.getPot(0);
        int selected_pattern = std::min((s1 * 32) / 128, Mode::NUM_PATTERNS - 1);  // Clamp to 0-31

        // Apply this pattern to all modes 1-14 (skip mode 0)
        for (int mode_num = 1; mode_num < Song::NUM_MODES; ++mode_num) {
            mode_pattern_overrides_[mode_num] = selected_pattern;
        }

        // S2: Scale root (0-127 maps to 0-11)
        uint8_t s2 = event.getPot(1);
        global_scale_root_ = std::min((s2 * 12) / 128, 11);  // Clamp to 0-11

        // S3: Scale type (0-127 maps to 0-7, assuming 8 scale types)
        uint8_t s3 = event.getPot(2);
        global_scale_type_ = std::min((s3 * 8) / 128, 7);  // Clamp to 0-7

        // S4: Velocity offset (-64 to +63, map from 0-127)
        uint8_t s4 = event.getPot(3);
        int velocity_offset = std::clamp((int)s4 - 64, -64, 63);

        // Apply velocity offset to all modes
        for (int mode_num = 1; mode_num < Song::NUM_MODES; ++mode_num) {
            mode_velocity_offsets_[mode_num] = velocity_offset;
        }
    }
    // If step is inactive (button off), keep previous pattern (no override change)
}

void Mode0Sequencer::parseEvent(const Event& event, int target_mode) {
    // Bounds check on target_mode
    if (target_mode < 0 || target_mode >= Song::NUM_MODES) {
        std::cerr << "[Mode0Sequencer] Invalid target_mode: " << target_mode << std::endl;
        return;
    }

    if (!event.getSwitch()) {
        return;  // Event is off, don't override
    }

    // S1: Pattern (0-127 maps to 0-31)
    uint8_t s1 = event.getPot(0);
    int pattern = std::min((s1 * 32) / 128, Mode::NUM_PATTERNS - 1);  // Clamp to 0-31
    mode_pattern_overrides_[target_mode] = pattern;

    // S2: Scale root (0-127 maps to 0-11)
    uint8_t s2 = event.getPot(1);
    global_scale_root_ = std::min((s2 * 12) / 128, 11);  // Clamp to 0-11

    // S3: Scale type (0-127 maps to 0-7)
    uint8_t s3 = event.getPot(2);
    global_scale_type_ = std::min((s3 * 8) / 128, 7);  // Clamp to 0-7

    // S4: Velocity offset (-64 to +63, map from 0-127)
    uint8_t s4 = event.getPot(3);
    int velocity_offset = std::clamp((int)s4 - 64, -64, 63);
    mode_velocity_offsets_[target_mode] = velocity_offset;
}

int Mode0Sequencer::getPatternOverride(int mode) const {
    // Bounds check
    if (mode < 0 || mode >= Song::NUM_MODES) {
        return -1;  // Invalid mode
    }
    return mode_pattern_overrides_[mode];
}

int Mode0Sequencer::getVelocityOffset(int mode) const {
    // Bounds check
    if (mode < 0 || mode >= Song::NUM_MODES) {
        return 0;  // Default to no offset
    }
    return mode_velocity_offsets_[mode];
}

} // namespace gruvbok
