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

    // LED pattern control (public enum for external access)
    enum class LEDPattern {
        TEMPO_BEAT,
        BUTTON_HELD,
        SAVING,
        LOADING,
        ERROR,
        MIRROR_MODE
    };
    void triggerLEDPattern(LEDPattern pattern, uint8_t brightness = 255);

    // Trigger LED pattern by name (for Lua API)
    void triggerLEDByName(const std::string& pattern_name, uint8_t brightness = 255);

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

    // Song mode (mode 0) - runs at 1/16th speed (each step = 1 full pattern)
    int song_mode_step_;     // Current step in Mode 0 (0-15, advances every 16 normal steps)

    uint32_t last_step_time_;
    uint32_t step_interval_ms_;

    // MIDI clock tracking (24 PPQN) - use absolute timing to prevent drift
    uint32_t clock_start_time_;     // When playback started (absolute time)
    uint32_t clock_pulse_count_;    // Number of clock pulses sent
    double clock_interval_ms_;      // Interval between clock pulses (float for precision)

    // LED tempo indicator with patterns
    LEDPattern led_pattern_;
    bool led_on_;
    uint8_t led_brightness_;  // 0-255 (for PWM hardware support)
    uint32_t led_state_start_time_;
    uint32_t led_phase_start_time_;
    int led_blink_count_;

    static constexpr uint32_t LED_TEMPO_DURATION_MS = 50;  // LED stays on for 50ms

    // Debounced Lua reinit when tempo changes
    bool lua_reinit_pending_;
    uint32_t last_tempo_change_time_;
    static constexpr uint32_t TEMPO_DEBOUNCE_MS = 1000;  // Wait 1 second after last tempo change

    void calculateStepInterval();
    void calculateClockInterval();
    void sendMidiClock();
    void processStep();
    void handleInput();
    void updateLED();
    void reinitLuaModes();  // Reinitialize all Lua modes with current tempo
};

} // namespace gruvbok
