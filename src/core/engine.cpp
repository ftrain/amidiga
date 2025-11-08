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
    , song_mode_step_(0)
    , song_mode_loop_length_(16)  // Default to 16 bars
    , target_mode_(1)  // Default target mode for Mode 0 editing
    , global_scale_root_(0)  // C
    , global_scale_type_(0)  // Ionian/Major
    , dirty_(false)
    , last_autosave_time_(0)
    , last_step_time_(0)
    , step_interval_ms_(0)
    , clock_start_time_(0)
    , clock_pulse_count_(0)
    , clock_interval_ms_(0.0)
    , led_pattern_(LEDPattern::TEMPO_BEAT)
    , led_on_(false)
    , led_brightness_(255)
    , led_state_start_time_(0)
    , led_phase_start_time_(0)
    , led_blink_count_(0)
    , lua_reinit_pending_(false)
    , last_tempo_change_time_(0) {

    // Initialize per-mode arrays
    for (int i = 0; i < Song::NUM_MODES; ++i) {
        mode_velocity_offsets_[i] = 0;
        mode_pattern_overrides_[i] = -1;  // -1 means use default pattern
    }

    scheduler_ = std::make_unique<MidiScheduler>(hardware);
    calculateStepInterval();
    calculateClockInterval();
    calculateMode0LoopLength();  // Calculate initial loop length from Mode 0

    // Set Engine instance on all loaded Lua modes for LED control
    if (mode_loader_) {
        mode_loader_->setEngine(this);
    }
}

void Engine::start() {
    is_playing_ = true;
    current_step_ = 0;
    song_mode_step_ = 0;  // Reset Mode 0 position
    last_step_time_ = hardware_->getMillis();

    // Reset MIDI clock timing (absolute timing to prevent drift)
    clock_start_time_ = last_step_time_;
    clock_pulse_count_ = 0;

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

    // Update LED tempo indicator
    updateLED();

    // Check for debounced Lua reinit
    if (lua_reinit_pending_) {
        uint32_t current_time = hardware_->getMillis();
        if (current_time - last_tempo_change_time_ >= TEMPO_DEBOUNCE_MS) {
            reinitLuaModes();
            lua_reinit_pending_ = false;
        }
    }

    // Check for autosave (dirty flag + 20 second timer)
    checkAutosave();

    // Handle input
    handleInput();

    if (!is_playing_) {
        return;
    }

    uint32_t current_time = hardware_->getMillis();

    // Send MIDI clock messages at 24 PPQN using absolute timing to prevent drift
    // Calculate when the next clock pulse should occur
    uint32_t next_clock_time = clock_start_time_ + (uint32_t)(clock_pulse_count_ * clock_interval_ms_);

    // Send clock pulses for all that are due (can catch up if we fell behind)
    while (current_time >= next_clock_time) {
        sendMidiClock();
        clock_pulse_count_++;
        next_clock_time = clock_start_time_ + (uint32_t)(clock_pulse_count_ * clock_interval_ms_);
    }

    // Check if it's time for next step
    if (current_time - last_step_time_ >= step_interval_ms_) {
        processStep();
        last_step_time_ = current_time;

        // Advance step
        current_step_ = (current_step_ + 1) % 16;

        // Mode 0 runs at 1/16th speed: advance song_mode_step_ when current_step_ wraps to 0
        if (current_step_ == 0) {
            song_mode_step_ = (song_mode_step_ + 1) % song_mode_loop_length_;
        }
    }
}

void Engine::setTempo(int bpm) {
    tempo_ = std::clamp(bpm, 1, 1000);
    calculateStepInterval();
    calculateClockInterval();

    // Mark Lua reinit as pending (debounced)
    lua_reinit_pending_ = true;
    last_tempo_change_time_ = hardware_->getMillis();
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
    markDirty();
}

void Engine::setCurrentPot(int pot, uint8_t value) {
    if (pot < 0 || pot >= 4) return;

    Mode& mode = song_->getMode(current_mode_);
    Pattern& pattern = mode.getPattern(current_pattern_);
    Event& event = pattern.getEvent(current_track_, current_step_);

    event.setPot(pot, value);
    markDirty();
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
    // At 120 BPM: 60000 / 120 / 24 = 20.833333... ms per clock
    // Use double precision to prevent rounding errors
    clock_interval_ms_ = (60000.0 / static_cast<double>(tempo_)) / 24.0;
}

void Engine::sendMidiClock() {
    scheduler_->sendClock();
}

void Engine::processStep() {
    // Parse Mode 0 parameters at the start of each bar (current_step_ == 0)
    // Mode 0 contains configuration for all modes: pattern, scale, velocity
    if (current_step_ == 0) {
        applyMode0Parameters();
    }

    // Determine which pattern to play for each mode
    // Mode 0 controls which pattern each mode plays via mode_pattern_overrides_
    for (int mode_num = 1; mode_num < Song::NUM_MODES; ++mode_num) {
        // Use pattern override if set, otherwise use current_pattern_
        int pattern_to_play = (mode_pattern_overrides_[mode_num] >= 0)
            ? mode_pattern_overrides_[mode_num]
            : current_pattern_;

        Mode& mode = song_->getMode(mode_num);
        Pattern& pattern = mode.getPattern(pattern_to_play);

        LuaContext* lua_mode = mode_loader_->getMode(mode_num);

        if (lua_mode && lua_mode->isValid()) {
            // Process all tracks for this mode
            for (int track = 0; track < Pattern::NUM_TRACKS; ++track) {
                const Event& event = pattern.getEvent(track, current_step_);

                // Call Lua to process event
                // TODO: Pass global scale and velocity offset to Lua
                auto midi_events = lua_mode->callProcessEvent(track, event);

                // Schedule returned MIDI events
                scheduler_->schedule(midi_events);
            }
        }
    }

    // LED tempo indicator: blink on every beat (every 4 steps)
    if (current_step_ % 4 == 0) {
        triggerLEDPattern(LEDPattern::TEMPO_BEAT);
    }
}

void Engine::handleInput() {
    // Read rotary pots for global controls
    uint8_t r1 = hardware_->readRotaryPot(0);  // Mode: 0-14
    uint8_t r2 = hardware_->readRotaryPot(1);  // Tempo: 0-1000
    uint8_t r3 = hardware_->readRotaryPot(2);  // Pattern: 0-31
    uint8_t r4 = hardware_->readRotaryPot(3);  // Track OR target mode (when in Mode 0)

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

    // Map R4: In Mode 0, it selects target mode (1-14). Otherwise, it selects track (0-7).
    if (current_mode_ == 0) {
        // Mode 0: R4 selects target mode (1-14)
        int new_target_mode = 1 + (r4 * 14) / 128;  // Map to 1-14
        if (new_target_mode != target_mode_) {
            target_mode_ = new_target_mode;
        }
    } else {
        // Other modes: R4 selects track (0-7)
        int new_track = (r4 * 8) / 128;
        if (new_track != current_track_) {
            setTrack(new_track);
        }
    }

    // Read buttons (B1-B16) to toggle steps
    // When a button is pressed, parameter-lock the current slider values to that event
    for (int btn = 0; btn < 16; ++btn) {
        if (hardware_->readButton(btn)) {
            // In Mode 0, buttons write to Mode 0 Pattern 0 using target_mode_ as "track"
            // In other modes, buttons write to current mode/pattern/track
            int edit_mode, edit_pattern, edit_track;

            if (current_mode_ == 0) {
                // Mode 0: Always edit Mode 0, Pattern 0, use target_mode_ as "track"
                edit_mode = 0;
                edit_pattern = 0;
                edit_track = target_mode_;  // target_mode_ is 1-14, but we need 0-13 for track index
                if (edit_track < 1 || edit_track >= Song::NUM_MODES) {
                    edit_track = 1;  // Safety clamp
                }
                edit_track -= 1;  // Convert to 0-13
            } else {
                // Normal mode: edit current mode/pattern/track
                edit_mode = current_mode_;
                edit_pattern = current_pattern_;
                edit_track = current_track_;
            }

            // Toggle the event
            Mode& mode = song_->getMode(edit_mode);
            Pattern& pattern = mode.getPattern(edit_pattern);
            Event& event = pattern.getEvent(edit_track, btn);

            // Toggle switch
            event.setSwitch(!event.getSwitch());

            // If we just turned it ON, parameter-lock current slider values to this event
            if (event.getSwitch()) {
                for (int pot = 0; pot < 4; ++pot) {
                    uint8_t value = hardware_->readSliderPot(pot);
                    event.setPot(pot, value);
                }
            }

            // Mark dirty and recalculate Mode 0 loop length if in Mode 0
            markDirty();
            if (current_mode_ == 0) {
                calculateMode0LoopLength();
            }
        }
    }

    // NOTE: We no longer continuously write slider values to the current step.
    // Slider values are only saved when you press a button to create an event.
}

void Engine::triggerLEDPattern(LEDPattern pattern, uint8_t brightness) {
    led_pattern_ = pattern;
    led_brightness_ = brightness;
    led_state_start_time_ = hardware_->getMillis();
    led_phase_start_time_ = led_state_start_time_;
    led_blink_count_ = 0;
    led_on_ = true;
    hardware_->setLED(true);
}

void Engine::triggerLEDByName(const std::string& pattern_name, uint8_t brightness) {
    LEDPattern pattern = LEDPattern::TEMPO_BEAT;

    if (pattern_name == "tempo") pattern = LEDPattern::TEMPO_BEAT;
    else if (pattern_name == "held") pattern = LEDPattern::BUTTON_HELD;
    else if (pattern_name == "saving") pattern = LEDPattern::SAVING;
    else if (pattern_name == "loading") pattern = LEDPattern::LOADING;
    else if (pattern_name == "error") pattern = LEDPattern::ERROR;
    else if (pattern_name == "mirror") pattern = LEDPattern::MIRROR_MODE;

    triggerLEDPattern(pattern, brightness);
}

void Engine::updateLED() {
    uint32_t current_time = hardware_->getMillis();
    uint32_t pattern_elapsed = current_time - led_state_start_time_;
    uint32_t phase_elapsed = current_time - led_phase_start_time_;

    switch (led_pattern_) {
        case LEDPattern::TEMPO_BEAT:
            // Simple 50ms pulse
            if (led_on_ && phase_elapsed >= LED_TEMPO_DURATION_MS) {
                hardware_->setLED(false);
                led_on_ = false;
            }
            break;

        case LEDPattern::BUTTON_HELD:
            // Fast double-blink: 100ms on, 50ms off, 100ms on, 150ms off (repeat)
            if (pattern_elapsed < 100) {
                if (!led_on_) {
                    hardware_->setLED(true);
                    led_on_ = true;
                }
            } else if (pattern_elapsed < 150) {
                if (led_on_) {
                    hardware_->setLED(false);
                    led_on_ = false;
                }
            } else if (pattern_elapsed < 250) {
                if (!led_on_) {
                    hardware_->setLED(true);
                    led_on_ = true;
                }
            } else if (pattern_elapsed < 400) {
                if (led_on_) {
                    hardware_->setLED(false);
                    led_on_ = false;
                }
            } else {
                // Restart pattern
                led_state_start_time_ = current_time;
            }
            break;

        case LEDPattern::SAVING:
            // Rapid blinks: 100ms on/off, 5 times total (1 second)
            {
                int cycle = (int)(phase_elapsed / 200);  // Each cycle is 200ms
                if (cycle >= 5) {
                    // Pattern complete, return to tempo
                    led_pattern_ = LEDPattern::TEMPO_BEAT;
                    hardware_->setLED(false);
                    led_on_ = false;
                } else {
                    bool should_be_on = (phase_elapsed % 200) < 100;
                    if (should_be_on != led_on_) {
                        hardware_->setLED(should_be_on);
                        led_on_ = should_be_on;
                    }
                }
            }
            break;

        case LEDPattern::LOADING:
            // Slow pulse: 1 second on, 1 second off (2 second cycle)
            {
                bool should_be_on = (pattern_elapsed % 2000) < 1000;
                if (should_be_on != led_on_) {
                    hardware_->setLED(should_be_on);
                    led_on_ = should_be_on;
                }
            }
            break;

        case LEDPattern::ERROR:
            // Triple fast blink: 50ms on/off, 3 times (300ms total)
            {
                int cycle = (int)(phase_elapsed / 100);  // Each cycle is 100ms
                if (cycle >= 3) {
                    // Pattern complete, return to tempo
                    led_pattern_ = LEDPattern::TEMPO_BEAT;
                    hardware_->setLED(false);
                    led_on_ = false;
                } else {
                    bool should_be_on = (phase_elapsed % 100) < 50;
                    if (should_be_on != led_on_) {
                        hardware_->setLED(should_be_on);
                        led_on_ = should_be_on;
                    }
                }
            }
            break;

        case LEDPattern::MIRROR_MODE:
            // Alternating long/short: 200ms on, 100ms off (repeat)
            if (pattern_elapsed < 200) {
                if (!led_on_) {
                    hardware_->setLED(true);
                    led_on_ = true;
                }
            } else if (pattern_elapsed < 300) {
                if (led_on_) {
                    hardware_->setLED(false);
                    led_on_ = false;
                }
            } else {
                // Restart pattern
                led_state_start_time_ = current_time;
            }
            break;
    }
}

void Engine::reinitLuaModes() {
    // Reinitialize all Lua modes with current tempo
    // This is called after tempo changes (debounced)
    std::cout << "Reinitializing Lua modes with tempo=" << tempo_ << " BPM" << std::endl;

    LuaInitContext context;
    context.tempo = tempo_;

    for (int mode_num = 0; mode_num < Song::NUM_MODES; ++mode_num) {
        LuaContext* lua_mode = mode_loader_->getMode(mode_num);
        if (lua_mode && lua_mode->isValid()) {
            context.mode_number = mode_num;
            context.midi_channel = mode_num;  // Each mode on its own channel
            lua_mode->callInit(context);
        }
    }
}

// ============================================================================
// Audio Output Control
// ============================================================================

bool Engine::initAudioOutput(const std::string& soundfont_path) {
    // Create AudioOutput if not already created
    if (!audio_output_) {
        audio_output_ = std::make_unique<AudioOutput>();
    }

    // Initialize FluidSynth
    if (!audio_output_->init()) {
        std::cerr << "[Engine] Failed to initialize audio output\n";
        return false;
    }

    // Load SoundFont if provided
    if (!soundfont_path.empty()) {
        if (!audio_output_->loadSoundFont(soundfont_path)) {
            std::cerr << "[Engine] Failed to load SoundFont: " << soundfont_path << "\n";
            return false;
        }
    }

    // Connect to scheduler
    scheduler_->setAudioOutput(audio_output_.get());

    std::cout << "[Engine] Audio output initialized successfully\n";
    return true;
}

void Engine::setUseInternalAudio(bool use_internal) {
    scheduler_->setUseInternalAudio(use_internal);
    if (use_internal && audio_output_ && audio_output_->isReady()) {
        std::cout << "[Engine] Using internal audio (FluidSynth)\n";
    }
}

void Engine::setUseExternalMIDI(bool use_external) {
    scheduler_->setUseExternalMIDI(use_external);
    if (use_external) {
        std::cout << "[Engine] Using external MIDI\n";
    }
}

bool Engine::isUsingInternalAudio() const {
    return scheduler_->isUsingInternalAudio();
}

bool Engine::isUsingExternalMIDI() const {
    return scheduler_->isUsingExternalMIDI();
}

bool Engine::isAudioOutputReady() const {
    return audio_output_ && audio_output_->isReady();
}

void Engine::setAudioGain(float gain) {
    if (audio_output_) {
        audio_output_->setGain(gain);
    }
}

float Engine::getAudioGain() const {
    if (audio_output_) {
        return audio_output_->getGain();
    }
    return 0.0f;
}

// ============================================================================
// Mode 0 Helpers
// ============================================================================

void Engine::calculateMode0LoopLength() {
    // Scan Mode 0, Pattern 0, all tracks (0-13 map to modes 1-14)
    // Find the highest step number with switch on
    Mode& mode0 = song_->getMode(0);
    Pattern& pattern = mode0.getPattern(0);

    int max_step = 0;  // Default to 1 bar minimum
    for (int track = 0; track < 14; ++track) {  // Tracks 0-13 represent modes 1-14
        for (int step = 0; step < 16; ++step) {
            const Event& event = pattern.getEvent(track, step);
            if (event.getSwitch() && step > max_step) {
                max_step = step;
            }
        }
    }

    // Loop length is max_step + 1 (e.g., if B4 is pressed, max_step=3, loop_length=4)
    song_mode_loop_length_ = max_step + 1;

    // If no buttons pressed, default to 1 bar
    if (song_mode_loop_length_ < 1) {
        song_mode_loop_length_ = 1;
    }
}

void Engine::parseMode0Event(const Event& event, int target_mode) {
    // Extract parameters from Mode 0 event:
    // S1: Pattern (0-31)
    // S2: Scale root (0-11, C-B)
    // S3: Scale type (0-N)
    // S4: Velocity offset (-64 to +63)

    if (!event.getSwitch()) {
        return;  // Event is off, don't override
    }

    // S1: Pattern (0-127 maps to 0-31)
    uint8_t s1 = event.getPot(0);
    int pattern = (s1 * 32) / 128;
    mode_pattern_overrides_[target_mode] = pattern;

    // S2: Scale root (0-127 maps to 0-11)
    uint8_t s2 = event.getPot(1);
    global_scale_root_ = (s2 * 12) / 128;

    // S3: Scale type (0-127 maps to 0-N, TBD how many scale types)
    uint8_t s3 = event.getPot(2);
    global_scale_type_ = (s3 * 8) / 128;  // Assume 8 scale types for now

    // S4: Velocity offset (-64 to +63, map from 0-127)
    uint8_t s4 = event.getPot(3);
    int velocity_offset = (int)s4 - 64;  // Map 0-127 to -64 to +63
    mode_velocity_offsets_[target_mode] = velocity_offset;
}

void Engine::applyMode0Parameters() {
    // Read Mode 0 events and apply parameters for all modes
    // Mode 0 Pattern 0 contains configuration for modes 1-14 (tracks 0-13)
    Mode& mode0 = song_->getMode(0);
    Pattern& pattern = mode0.getPattern(0);

    // Parse events for all modes at the current song_mode_step_
    for (int track = 0; track < 14; ++track) {  // Tracks 0-13 represent modes 1-14
        int target_mode = track + 1;  // Convert track to mode number (1-14)
        const Event& event = pattern.getEvent(track, song_mode_step_);
        parseMode0Event(event, target_mode);
    }
}

// ============================================================================
// Autosave
// ============================================================================

void Engine::markDirty() {
    dirty_ = true;
}

void Engine::checkAutosave() {
    if (!dirty_) {
        return;  // Nothing to save
    }

    uint32_t current_time = hardware_->getMillis();
    if (current_time - last_autosave_time_ >= AUTOSAVE_INTERVAL_MS) {
        // Perform autosave (binary format for flash efficiency)
        std::string save_path = "/tmp/gruvbok_autosave.bin";

        // Trigger LED pattern for saving
        triggerLEDPattern(LEDPattern::SAVING);

        if (song_->saveBinary(save_path)) {
            std::cout << "[Autosave] Saved to " << save_path << " (binary format)" << std::endl;
            dirty_ = false;
            last_autosave_time_ = current_time;
        } else {
            std::cerr << "[Autosave] Failed to save to " << save_path << std::endl;
            triggerLEDPattern(LEDPattern::ERROR);
        }
    }
}

} // namespace gruvbok
