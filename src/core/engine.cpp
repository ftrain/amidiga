#include "engine.h"
#include <iostream>
#include <algorithm>

namespace gruvbok {

Engine::Engine(Song* song, HardwareInterface* hardware, ModeLoader* mode_loader)
    : song_(song)
    , hardware_(hardware)
    , mode_loader_(mode_loader)
    , dirty_(false)
    , last_autosave_time_(0) {

    // Set sensible default instruments for each mode (General MIDI)
    mode_programs_[0] = 0;    // Mode 0: Song sequencer (no MIDI output)
    mode_programs_[1] = 48;   // Mode 1: Chords → String Ensemble
    mode_programs_[2] = 33;   // Mode 2: Acid Bassline → Electric Bass (finger)
    mode_programs_[3] = 38;   // Mode 3: Cellular Automata → Synth Bass 1
    mode_programs_[4] = 81;   // Mode 4: Arpeggiator → Sawtooth Lead
    mode_programs_[5] = 24;   // Mode 5: Euclidean → Acoustic Guitar (nylon)
    mode_programs_[6] = 88;   // Mode 6: Random → New Age Pad
    mode_programs_[7] = 56;   // Mode 7: Sample & Hold → Trumpet
    mode_programs_[8] = 4;    // Mode 8: Drunk Walk → Electric Piano 1
    mode_programs_[9] = 81;   // Mode 9: Wavetable → Sawtooth Lead
    mode_programs_[10] = 0;   // Mode 10: Drums → GM Drums (channel 10, program ignored)
    mode_programs_[11] = 40;  // Mode 11: Violin
    mode_programs_[12] = 16;  // Mode 12: Drawbar Organ
    mode_programs_[13] = 65;  // Mode 13: Alto Sax
    mode_programs_[14] = 98;  // Mode 14: Crystal (FX)

    // Create components
    scheduler_ = std::make_unique<MidiScheduler>(hardware);
    led_controller_ = std::make_unique<LEDController>(hardware);
    clock_manager_ = std::make_unique<MidiClockManager>(scheduler_.get(), hardware);
    mode0_sequencer_ = std::make_unique<Mode0Sequencer>(song);
    playback_state_ = std::make_unique<PlaybackState>(hardware);

    // Calculate initial Mode 0 loop length
    mode0_sequencer_->calculateLoopLength();

    // Set Engine instance on all loaded Lua modes for LED control
    if (mode_loader_) {
        mode_loader_->setEngine(this);
    }
}

void Engine::start() {
    playback_state_->start();
    mode0_sequencer_->start();
    clock_manager_->start();

    // Initialize Lua modes and send Program Change messages for all instruments
    reinitLuaModes();
}

void Engine::stop() {
    playback_state_->stop();
    clock_manager_->stop();
    scheduler_->clear();
}

void Engine::update() {
    // Update MIDI scheduler
    scheduler_->update();

    // Update LED controller
    led_controller_->update();

    // Update MIDI clock
    clock_manager_->update();

    // Check for debounced Lua reinit
    uint32_t current_time = hardware_->getMillis();
    if (playback_state_->isLuaReinitPending(current_time)) {
        reinitLuaModes();
        playback_state_->clearLuaReinitPending();
    }

    // Check for autosave (dirty flag + 20 second timer)
    checkAutosave();

    // Handle input
    handleInput();

    if (!playback_state_->isPlaying()) {
        return;
    }

    // Check if it's time for next step
    if (playback_state_->shouldAdvanceStep(current_time)) {
        processStep();
        playback_state_->advanceStep(current_time);

        // Mode 0 runs at 1/16th speed: advance song_mode_step_ when current_step_ wraps to 0
        if (playback_state_->getCurrentStep() == 0) {
            mode0_sequencer_->advanceStep();
        }
    }
}

void Engine::setTempo(int bpm) {
    playback_state_->setTempo(bpm);
    clock_manager_->setTempo(bpm);
}

void Engine::setMode(int mode) {
    playback_state_->setMode(mode);
}

void Engine::setPattern(int pattern) {
    playback_state_->setPattern(pattern);
}

void Engine::setTrack(int track) {
    playback_state_->setTrack(track);
}

// Getter implementations (delegate to components)
bool Engine::isPlaying() const {
    return playback_state_->isPlaying();
}

int Engine::getTempo() const {
    return playback_state_->getTempo();
}

int Engine::getCurrentMode() const {
    return playback_state_->getCurrentMode();
}

int Engine::getCurrentPattern() const {
    return playback_state_->getCurrentPattern();
}

int Engine::getCurrentTrack() const {
    return playback_state_->getCurrentTrack();
}

int Engine::getCurrentStep() const {
    return playback_state_->getCurrentStep();
}

int Engine::getSongModeStep() const {
    return mode0_sequencer_->getCurrentStep();
}

int Engine::getTargetMode() const {
    return playback_state_->getTargetMode();
}

void Engine::toggleCurrentSwitch() {
    Mode& mode = song_->getMode(playback_state_->getCurrentMode());
    Pattern& pattern = mode.getPattern(playback_state_->getCurrentPattern());
    Event& event = pattern.getEvent(playback_state_->getCurrentTrack(), playback_state_->getCurrentStep());

    event.setSwitch(!event.getSwitch());
    markDirty();
}

void Engine::setCurrentPot(int pot, uint8_t value) {
    if (pot < 0 || pot >= 4) return;

    Mode& mode = song_->getMode(playback_state_->getCurrentMode());
    Pattern& pattern = mode.getPattern(playback_state_->getCurrentPattern());
    Event& event = pattern.getEvent(playback_state_->getCurrentTrack(), playback_state_->getCurrentStep());

    event.setPot(pot, value);
    markDirty();
}

void Engine::setEventPot(int mode, int pattern, int track, int step, int pot, uint8_t value) {
    // Bounds checking
    if (mode < 0 || mode >= Song::NUM_MODES) return;
    if (pattern < 0 || pattern >= Mode::NUM_PATTERNS) return;
    if (track < 0 || track >= Pattern::NUM_TRACKS) return;
    if (step < 0 || step >= Track::NUM_EVENTS) return;
    if (pot < 0 || pot >= 4) return;

    // Get the event and set the pot value directly
    Mode& m = song_->getMode(mode);
    Pattern& p = m.getPattern(pattern);
    Event& e = p.getEvent(track, step);

    e.setPot(pot, value);
    markDirty();
}

void Engine::processStep() {
    int current_step = playback_state_->getCurrentStep();
    int current_mode = playback_state_->getCurrentMode();
    int current_pattern = playback_state_->getCurrentPattern();

    // Parse Mode 0 parameters at the start of each bar (current_step == 0)
    // Only apply Mode 0 pattern sequence when in Mode 0
    if (current_step == 0 && current_mode == 0) {
        mode0_sequencer_->applyParameters();
    }

    // Determine which pattern to play for each mode
    // Mode 0: Follow pattern sequence from mode0_sequencer_
    // Modes 1-15: Loop current_pattern only (for editing)
    for (int mode_num = 1; mode_num < Song::NUM_MODES; ++mode_num) {
        int pattern_to_play;

        if (current_mode == 0) {
            // In Mode 0: Use pattern override if set, otherwise use current_pattern
            int override = mode0_sequencer_->getPatternOverride(mode_num);
            pattern_to_play = (override >= 0) ? override : current_pattern;
        } else {
            // In edit modes (1-15): Always loop current_pattern (ignore Mode 0 sequence)
            pattern_to_play = current_pattern;
        }

        Mode& mode = song_->getMode(mode_num);
        Pattern& pattern = mode.getPattern(pattern_to_play);

        LuaContext* lua_mode = mode_loader_->getMode(mode_num);

        if (lua_mode && lua_mode->isValid()) {
            // Process all tracks for this mode
            for (int track = 0; track < Pattern::NUM_TRACKS; ++track) {
                const Event& event = pattern.getEvent(track, current_step);

                // Call Lua to process event
                // TODO: Pass global scale and velocity offset to Lua
                auto midi_events = lua_mode->callProcessEvent(track, event);

                // Schedule returned MIDI events
                scheduler_->schedule(midi_events);
            }
        }
    }

    // LED tempo indicator: blink on every beat (every 4 steps)
    if (current_step % 4 == 0) {
        led_controller_->triggerPattern(LEDPattern::TEMPO_BEAT);
    }
}

void Engine::handleInput() {
    // Read rotary pots for global controls
    uint8_t r1 = hardware_->readRotaryPot(0);  // Mode: 0-14
    uint8_t r2 = hardware_->readRotaryPot(1);  // Tempo: 0-1000
    uint8_t r3 = hardware_->readRotaryPot(2);  // Pattern: 0-31
    uint8_t r4 = hardware_->readRotaryPot(3);  // Track OR target mode (when in Mode 0)

    // Map R1 to mode (0-127 -> 0-14)
    int new_mode = std::min((r1 * 15) / 128, 14);
    if (new_mode != playback_state_->getCurrentMode()) {
        setMode(new_mode);
    }

    // Map R2 to tempo (0-127 -> 60-240 BPM for now)
    int new_tempo = 60 + (r2 * 180) / 127;
    if (std::abs(new_tempo - playback_state_->getTempo()) > 5) {  // Hysteresis
        setTempo(new_tempo);
    }

    // Map R3 to pattern (0-127 -> 0-31)
    int new_pattern = std::min((r3 * 32) / 128, 31);
    if (new_pattern != playback_state_->getCurrentPattern()) {
        setPattern(new_pattern);
    }

    // Map R4: In Mode 0, it selects target mode (1-14). Otherwise, it selects track (0-7).
    int current_mode = playback_state_->getCurrentMode();
    if (current_mode == 0) {
        // Mode 0: R4 selects target mode (1-14)
        int new_target_mode = std::min(1 + (r4 * 14) / 128, 14);  // Map to 1-14
        if (new_target_mode != playback_state_->getTargetMode()) {
            playback_state_->setTargetMode(new_target_mode);
        }
    } else {
        // Other modes: R4 selects track (0-7)
        int new_track = std::min((r4 * 8) / 128, 7);
        if (new_track != playback_state_->getCurrentTrack()) {
            setTrack(new_track);
        }
    }

    // Read buttons (B1-B16) to toggle steps
    // When a button is pressed, parameter-lock the current slider values to that event
    for (int btn = 0; btn < 16; ++btn) {
        if (hardware_->readButton(btn)) {
            // In Mode 0, buttons write to Mode 0 Pattern 0 Track 0 (pattern sequence)
            // In other modes, buttons write to current mode/pattern/track
            int edit_mode, edit_pattern, edit_track;

            if (current_mode == 0) {
                // Mode 0: Always edit Mode 0, Pattern 0, Track 0
                // All 16 buttons program the pattern sequence on Track 0
                edit_mode = 0;
                edit_pattern = 0;
                edit_track = 0;  // Mode 0 only uses Track 0
            } else {
                // Normal mode: edit current mode/pattern/track
                edit_mode = playback_state_->getCurrentMode();
                edit_pattern = playback_state_->getCurrentPattern();
                edit_track = playback_state_->getCurrentTrack();
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
            if (current_mode == 0) {
                mode0_sequencer_->calculateLoopLength();
            }
        }
    }

    // NOTE: We no longer continuously write slider values to the current step.
    // Slider values are only saved when you press a button to create an event.
}

void Engine::triggerLEDPattern(LEDPattern pattern, uint8_t brightness) {
    led_controller_->triggerPattern(pattern, brightness);
}

void Engine::triggerLEDByName(const std::string& pattern_name, uint8_t brightness) {
    led_controller_->triggerPatternByName(pattern_name, brightness);
}

void Engine::reinitLuaModes() {
    // Reinitialize all Lua modes with current tempo and Mode 0 context
    // This is called after tempo changes (debounced)
    int tempo = playback_state_->getTempo();
    std::cout << "Reinitializing Lua modes with tempo=" << tempo << " BPM" << std::endl;

    LuaInitContext context;
    context.tempo = tempo;

    for (int mode_num = 0; mode_num < Song::NUM_MODES; ++mode_num) {
        LuaContext* lua_mode = mode_loader_->getMode(mode_num);
        if (lua_mode && lua_mode->isValid()) {
            context.mode_number = mode_num;
            // Mode 0 produces no MIDI output (it's the song sequencer)
            // Modes 1-15 output on MIDI channels 0-14 (displayed as channels 1-15)
            context.midi_channel = (mode_num > 0) ? mode_num - 1 : 0;
            context.scale_root = mode0_sequencer_->getScaleRoot();
            context.scale_type = mode0_sequencer_->getScaleType();
            context.velocity_offset = mode0_sequencer_->getVelocityOffset(mode_num);
            lua_mode->callInit(context);

            // Send Program Change message to set instrument for this mode
            if (mode_num > 0) {  // Skip Mode 0 (no MIDI output)
                uint8_t program = mode_programs_[mode_num];
                uint8_t channel = mode_num - 1;  // Mode N → MIDI channel N-1 (Mode 1 → Ch 0, displayed as Ch 1)

                // Create Program Change MIDI message (0xC0 + channel)
                std::vector<uint8_t> program_change = {
                    static_cast<uint8_t>(0xC0 | (channel & 0x0F)),
                    program
                };

                MidiMessage msg(program_change, 0);
                hardware_->sendMidiMessage(msg);
            }
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
// Mode 0 Helpers (delegated to Mode0Sequencer)
// ============================================================================

void Engine::calculateMode0LoopLength() {
    // Delegate to mode0_sequencer_
    mode0_sequencer_->calculateLoopLength();
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
        led_controller_->triggerPattern(LEDPattern::SAVING);

        if (song_->saveBinary(save_path)) {
            std::cout << "[Autosave] Saved to " << save_path << " (binary format)" << std::endl;
            dirty_ = false;
            last_autosave_time_ = current_time;
        } else {
            std::cerr << "[Autosave] Failed to save to " << save_path << std::endl;
            led_controller_->triggerPattern(LEDPattern::ERROR);
        }
    }
}

// ============================================================================
// MIDI Program Mapping
// ============================================================================

void Engine::setModeProgram(int mode, uint8_t program) {
    if (mode < 0 || mode >= Song::NUM_MODES) {
        return;
    }

    mode_programs_[mode] = program;

    // Send Program Change message immediately if this mode is active
    if (mode > 0) {  // Skip Mode 0 (no MIDI output)
        uint8_t channel = mode - 1;  // Mode N → MIDI channel N-1 (Mode 1 → Ch 0, displayed as Ch 1)

        // Create Program Change MIDI message (0xC0 + channel)
        std::vector<uint8_t> program_change = {
            static_cast<uint8_t>(0xC0 | (channel & 0x0F)),
            program
        };

        MidiMessage msg(program_change, 0);
        hardware_->sendMidiMessage(msg);

        std::cout << "[Engine] Set Mode " << mode << " (channel " << static_cast<int>(channel)
                  << ") to program " << static_cast<int>(program) << std::endl;
    }

    markDirty();
}

uint8_t Engine::getModeProgram(int mode) const {
    if (mode < 0 || mode >= Song::NUM_MODES) {
        return 0;
    }
    return mode_programs_[mode];
}

} // namespace gruvbok
