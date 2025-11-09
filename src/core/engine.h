#pragma once

#include "song.h"
#include "led_controller.h"
#include "midi_clock_manager.h"
#include "mode0_sequencer.h"
#include "playback_state.h"
#include "../hardware/hardware_interface.h"
#include "../hardware/midi_scheduler.h"
#include "../hardware/audio_output.h"
#include "../lua_bridge/mode_loader.h"
#include <memory>

namespace gruvbok {

/**
 * Main playback engine
 * Loops through Events, calls Lua modes, schedules MIDI
 */
class Engine {
public:
    Engine(Song* song, HardwareInterface* hardware, ModeLoader* mode_loader);

    // Start/stop playback
    void start();
    void stop();
    bool isPlaying() const;

    // Main update loop - call frequently
    void update();

    // Global controls
    void setTempo(int bpm);  // 0-1000 BPM
    void setMode(int mode);  // 0-14
    void setPattern(int pattern);  // 0-31
    void setTrack(int track);  // 0-7

    int getTempo() const;
    int getCurrentMode() const;
    int getCurrentPattern() const;
    int getCurrentTrack() const;
    int getCurrentStep() const;
    int getSongModeStep() const;  // For Mode 0 visualization
    int getTargetMode() const;  // For Mode 0 target mode selection

    // MIDI Program mapping (instrument selection per mode)
    void setModeProgram(int mode, uint8_t program);  // Set GM program for a mode (0-127)
    uint8_t getModeProgram(int mode) const;  // Get GM program for a mode

    // Dirty flag (unsaved changes)
    bool isDirty() const { return dirty_; }
    void markDirty();
    void clearDirty() { dirty_ = false; }

    // Edit current event
    void toggleCurrentSwitch();
    void setCurrentPot(int pot, uint8_t value);

    // Direct event editing (for UI table)
    void setEventPot(int mode, int pattern, int track, int step, int pot, uint8_t value);

    // LED pattern control (delegated to LEDController)
    void triggerLEDPattern(LEDPattern pattern, uint8_t brightness = 255);
    void triggerLEDByName(const std::string& pattern_name, uint8_t brightness = 255);

    // Audio output control
    bool initAudioOutput(const std::string& soundfont_path = "");
    void setUseInternalAudio(bool use_internal);
    void setUseExternalMIDI(bool use_external);
    bool isUsingInternalAudio() const;
    bool isUsingExternalMIDI() const;
    bool isAudioOutputReady() const;
    void setAudioGain(float gain);
    float getAudioGain() const;

    // Mode 0 loop length calculation (public so it can be called after loading content)
    void calculateMode0LoopLength();

private:
    Song* song_;
    HardwareInterface* hardware_;
    ModeLoader* mode_loader_;
    std::unique_ptr<MidiScheduler> scheduler_;
    std::unique_ptr<AudioOutput> audio_output_;
    std::unique_ptr<LEDController> led_controller_;
    std::unique_ptr<MidiClockManager> clock_manager_;
    std::unique_ptr<Mode0Sequencer> mode0_sequencer_;
    std::unique_ptr<PlaybackState> playback_state_;

    // Per-mode MIDI program mapping (GM instruments, 0-127)
    uint8_t mode_programs_[Song::NUM_MODES];

    // Dirty flag and autosave
    bool dirty_;                 // True if data has been modified
    uint32_t last_autosave_time_;
    static constexpr uint32_t AUTOSAVE_INTERVAL_MS = 20000;  // 20 seconds

    void processStep();
    void handleInput();
    void reinitLuaModes();  // Reinitialize all Lua modes with current tempo

    // Autosave
    void checkAutosave();
};

} // namespace gruvbok
