#pragma once

#include "../hardware/hardware_interface.h"
#include "song.h"
#include <cstdint>
#include <algorithm>

namespace gruvbok {

/**
 * PlaybackState - Manages playback state and timing
 *
 * Handles tempo, current position (mode/pattern/track/step),
 * and step advancement timing.
 */
class PlaybackState {
public:
    explicit PlaybackState(HardwareInterface* hardware);

    /**
     * Start playback
     */
    void start();

    /**
     * Stop playback
     */
    void stop();

    /**
     * Check if it's time to advance to next step
     * @param current_time Current time in milliseconds
     * @return true if step should advance
     */
    bool shouldAdvanceStep(uint32_t current_time) const;

    /**
     * Advance to next step
     * Call this after shouldAdvanceStep() returns true
     * @param current_time Current time in milliseconds
     */
    void advanceStep(uint32_t current_time);

    /**
     * Set tempo (BPM) and recalculate step interval
     * @param bpm Beats per minute (1-1000)
     */
    void setTempo(int bpm);

    /**
     * Set current mode
     * @param mode Mode number (0-14)
     */
    void setMode(int mode);

    /**
     * Set current pattern
     * @param pattern Pattern number (0-31)
     */
    void setPattern(int pattern);

    /**
     * Set current track
     * @param track Track number (0-7)
     */
    void setTrack(int track);

    /**
     * Set target mode (for Mode 0 editing)
     * @param mode Target mode number (1-14)
     */
    void setTargetMode(int mode);

    // Getters
    bool isPlaying() const { return is_playing_; }
    int getTempo() const { return tempo_; }
    int getCurrentMode() const { return current_mode_; }
    int getCurrentPattern() const { return current_pattern_; }
    int getCurrentTrack() const { return current_track_; }
    int getCurrentStep() const { return current_step_; }
    int getTargetMode() const { return target_mode_; }
    uint32_t getStepIntervalMs() const { return step_interval_ms_; }

    /**
     * Check if Lua reinit is pending (debounced after tempo change)
     * @param current_time Current time in milliseconds
     * @return true if debounce period has elapsed and reinit is needed
     */
    bool isLuaReinitPending(uint32_t current_time) const;

    /**
     * Clear the Lua reinit pending flag
     */
    void clearLuaReinitPending();

private:
    HardwareInterface* hardware_;

    bool is_playing_;
    int tempo_;  // BPM
    int current_mode_;    // 0-14
    int current_pattern_; // 0-31
    int current_track_;   // 0-7
    int current_step_;    // 0-15
    int target_mode_;     // 1-14 (for Mode 0 target selection)

    uint32_t last_step_time_;
    uint32_t step_interval_ms_;

    // Debounced Lua reinit when tempo changes
    bool lua_reinit_pending_;
    uint32_t last_tempo_change_time_;
    static constexpr uint32_t TEMPO_DEBOUNCE_MS = 1000;  // Wait 1 second after last tempo change

    void calculateStepInterval();
};

} // namespace gruvbok
