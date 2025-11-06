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
    , song_mode_pattern_(0)
    , last_step_time_(0)
    , step_interval_ms_(0)
    , last_clock_time_(0)
    , clock_interval_ms_(0) {

    scheduler_ = std::make_unique<MidiScheduler>(hardware);
    calculateStepInterval();
    calculateClockInterval();
}

void Engine::start() {
    is_playing_ = true;
    current_step_ = 0;
    last_step_time_ = hardware_->getMillis();
    last_clock_time_ = last_step_time_;

    // Send MIDI start message
    scheduler_->sendStart();
}

void Engine::stop() {
    is_playing_ = false;
    scheduler_->clear();

    // Send MIDI stop message
    scheduler_->sendStop();
}

void Engine::update() {
    // Update MIDI scheduler
    scheduler_->update();

    // Handle input
    handleInput();

    if (!is_playing_) {
        return;
    }

    uint32_t current_time = hardware_->getMillis();

    // Send MIDI clock messages at 24 PPQN
    if (current_time - last_clock_time_ >= clock_interval_ms_) {
        sendMidiClock();
        last_clock_time_ = current_time;
    }

    // Check if it's time for next step
    if (current_time - last_step_time_ >= step_interval_ms_) {
        processStep();
        last_step_time_ = current_time;

        // Advance step
        current_step_ = (current_step_ + 1) % 16;

        // In song mode (mode 0), advance pattern when we loop back to step 0
        if (current_mode_ == 0 && current_step_ == 0) {
            song_mode_pattern_ = (song_mode_pattern_ + 1) % Mode::NUM_PATTERNS;
        }
    }
}

void Engine::setTempo(int bpm) {
    tempo_ = std::clamp(bpm, 1, 1000);
    calculateStepInterval();
    calculateClockInterval();
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

void Engine::calculateClockInterval() {
    // MIDI clock runs at 24 PPQN (pulses per quarter note)
    // Formula: (60000 / BPM) / 24 = ms per clock pulse
    // At 120 BPM: 60000 / 120 / 24 = 20.833ms per clock
    clock_interval_ms_ = (60000 / tempo_) / 24;
    if (clock_interval_ms_ < 1) clock_interval_ms_ = 1;  // Minimum 1ms
}

void Engine::sendMidiClock() {
    scheduler_->sendClock();
}

void Engine::processStep() {
    // Determine which pattern to play
    // Mode 0 = "song mode" - cycles through all 32 patterns
    // Modes 1-14 = play the current_pattern_ across all modes
    int pattern_to_play;

    if (current_mode_ == 0) {
        // Song mode: cycle through all patterns
        pattern_to_play = song_mode_pattern_;
    } else {
        // Edit mode: play the same pattern across all modes
        pattern_to_play = current_pattern_;
    }

    // Process ALL 15 modes simultaneously (each on its own MIDI channel!)
    for (int mode_num = 0; mode_num < Song::NUM_MODES; ++mode_num) {
        Mode& mode = song_->getMode(mode_num);
        Pattern& pattern = mode.getPattern(pattern_to_play);

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
    // When a button is pressed, parameter-lock the current slider values to that event
    for (int btn = 0; btn < 16; ++btn) {
        if (hardware_->readButton(btn)) {
            // Button pressed - toggle the switch for this step
            Mode& mode = song_->getMode(current_mode_);
            Pattern& pattern = mode.getPattern(current_pattern_);
            Event& event = pattern.getEvent(current_track_, btn);

            // Toggle switch
            event.setSwitch(!event.getSwitch());

            // If we just turned it ON, parameter-lock current slider values to this event
            if (event.getSwitch()) {
                for (int pot = 0; pot < 4; ++pot) {
                    uint8_t value = hardware_->readSliderPot(pot);
                    event.setPot(pot, value);
                }
            }
        }
    }

    // NOTE: We no longer continuously write slider values to the current step.
    // Slider values are only saved when you press a button to create an event.
}

} // namespace gruvbok
