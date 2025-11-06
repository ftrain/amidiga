#pragma once

#include "song.h"
#include "../hardware/hardware_interface.h"
#include "../hardware/midi_scheduler.h"
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
    bool isPlaying() const { return is_playing_; }

    // Main update loop - call frequently
    void update();

    // Global controls
    void setTempo(int bpm);  // 0-1000 BPM
    void setMode(int mode);  // 0-14
    void setPattern(int pattern);  // 0-31
    void setTrack(int track);  // 0-7

    int getTempo() const { return tempo_; }
    int getCurrentMode() const { return current_mode_; }
    int getCurrentPattern() const { return current_pattern_; }
    int getCurrentTrack() const { return current_track_; }
    int getCurrentStep() const { return current_step_; }

    // Edit current event
    void toggleCurrentSwitch();
    void setCurrentPot(int pot, uint8_t value);

private:
    Song* song_;
    HardwareInterface* hardware_;
    ModeLoader* mode_loader_;
    std::unique_ptr<MidiScheduler> scheduler_;

    bool is_playing_;
    int tempo_;  // BPM
    int current_mode_;
    int current_pattern_;
    int current_track_;
    int current_step_;  // 0-15

    // Song mode (mode 0) - cycles through all patterns
    int song_mode_pattern_;  // Current pattern in song mode

    uint32_t last_step_time_;
    uint32_t step_interval_ms_;

    // MIDI clock tracking (24 PPQN)
    uint32_t last_clock_time_;
    uint32_t clock_interval_ms_;

    // LED tempo indicator
    bool led_on_;
    uint32_t led_on_time_;
    static constexpr uint32_t LED_BLINK_DURATION_MS = 50;  // LED stays on for 50ms

    void calculateStepInterval();
    void calculateClockInterval();
    void sendMidiClock();
    void processStep();
    void handleInput();
    void updateLED();
};

} // namespace gruvbok
