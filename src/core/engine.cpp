#include "engine.h"
#include <iostream>
#include <algorithm>

namespace gruvbok {

Engine::Engine(Song* song, HardwareInterface* hardware, ModeLoader* mode_loader)
    : song_(song)
    , hardware_(hardware)
    , mode_loader_(mode_loader)
    , is_playing_(false)
    , tempo_(120)
    , current_mode_(1)  // Start with mode 1 (drums)
    , current_pattern_(0)
    , current_track_(0)
    , current_step_(0)
    , last_step_time_(0)
    , step_interval_ms_(0) {

    scheduler_ = std::make_unique<MidiScheduler>(hardware);
    calculateStepInterval();
}

void Engine::start() {
    is_playing_ = true;
    current_step_ = 0;
    last_step_time_ = hardware_->getMillis();
}

void Engine::stop() {
    is_playing_ = false;
    scheduler_->clear();
}

void Engine::update() {
    // Update MIDI scheduler
    scheduler_->update();

    // Handle input
    handleInput();

    if (!is_playing_) {
        return;
    }

    // Check if it's time for next step
    uint32_t current_time = hardware_->getMillis();
    if (current_time - last_step_time_ >= step_interval_ms_) {
        processStep();
        last_step_time_ = current_time;

        // Advance step
        current_step_ = (current_step_ + 1) % 16;
    }
}

void Engine::setTempo(int bpm) {
    tempo_ = std::clamp(bpm, 1, 1000);
    calculateStepInterval();
}

void Engine::setMode(int mode) {
    if (mode >= 0 && mode < Song::NUM_MODES) {
        current_mode_ = mode;
    }
}

void Engine::setPattern(int pattern) {
    if (pattern >= 0 && pattern < Mode::NUM_PATTERNS) {
        current_pattern_ = pattern;
    }
}

void Engine::setTrack(int track) {
    if (track >= 0 && track < Pattern::NUM_TRACKS) {
        current_track_ = track;
    }
}

void Engine::toggleCurrentSwitch() {
    Mode& mode = song_->getMode(current_mode_);
    Pattern& pattern = mode.getPattern(current_pattern_);
    Event& event = pattern.getEvent(current_track_, current_step_);

    event.setSwitch(!event.getSwitch());
    // No console spam
}

void Engine::setCurrentPot(int pot, uint8_t value) {
    if (pot < 0 || pot >= 4) return;

    Mode& mode = song_->getMode(current_mode_);
    Pattern& pattern = mode.getPattern(current_pattern_);
    Event& event = pattern.getEvent(current_track_, current_step_);

    event.setPot(pot, value);
    // No console spam
}

void Engine::calculateStepInterval() {
    // Calculate time per step in milliseconds
    // At 120 BPM: 1 beat = 500ms, 16 steps per bar = 4 beats, so 1 step = 125ms
    // Formula: (60000 / BPM) / 4 = ms per step (assuming 16th notes)
    step_interval_ms_ = (60000 / tempo_) / 4;
}

void Engine::processStep() {
    // Process ALL 15 modes simultaneously (each on its own MIDI channel!)
    for (int mode_num = 0; mode_num < Song::NUM_MODES; ++mode_num) {
        Mode& mode = song_->getMode(mode_num);
        Pattern& pattern = mode.getPattern(current_pattern_);

        LuaContext* lua_mode = mode_loader_->getMode(mode_num);

        if (lua_mode && lua_mode->isValid()) {
            // Process all tracks for this mode
            for (int track = 0; track < Pattern::NUM_TRACKS; ++track) {
                const Event& event = pattern.getEvent(track, current_step_);

                // Call Lua to process event
                auto midi_events = lua_mode->callProcessEvent(track, event);

                // Schedule returned MIDI events
                scheduler_->schedule(midi_events);
            }
        }
    }

    // Visual feedback on current mode only
    if (current_step_ % 4 == 0) {
        hardware_->setLED(true);
    }
}

void Engine::handleInput() {
    // Read rotary pots for global controls
    uint8_t r1 = hardware_->readRotaryPot(0);  // Mode: 0-14
    uint8_t r2 = hardware_->readRotaryPot(1);  // Tempo: 0-1000
    uint8_t r3 = hardware_->readRotaryPot(2);  // Pattern: 0-31
    uint8_t r4 = hardware_->readRotaryPot(3);  // Track: 0-7

    // Map R1 to mode (0-127 -> 0-14)
    int new_mode = (r1 * 15) / 128;
    if (new_mode != current_mode_) {
        setMode(new_mode);
    }

    // Map R2 to tempo (0-127 -> 60-240 BPM for now)
    int new_tempo = 60 + (r2 * 180) / 127;
    if (std::abs(new_tempo - tempo_) > 5) {  // Hysteresis
        setTempo(new_tempo);
    }

    // Map R3 to pattern (0-127 -> 0-31)
    int new_pattern = (r3 * 32) / 128;
    if (new_pattern != current_pattern_) {
        setPattern(new_pattern);
    }

    // Map R4 to track (0-127 -> 0-7)
    int new_track = (r4 * 8) / 128;
    if (new_track != current_track_) {
        setTrack(new_track);
    }

    // Read buttons (B1-B16) to toggle steps
    for (int btn = 0; btn < 16; ++btn) {
        if (hardware_->readButton(btn)) {
            // Button pressed - toggle the switch for this step
            int old_step = current_step_;
            current_step_ = btn;
            toggleCurrentSwitch();
            current_step_ = old_step;
        }
    }

    // Read sliders for pot values
    for (int pot = 0; pot < 4; ++pot) {
        uint8_t value = hardware_->readSliderPot(pot);
        setCurrentPot(pot, value);
    }
}

} // namespace gruvbok
