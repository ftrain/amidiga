#pragma once

#include "song.h"
#include <cstdint>

namespace gruvbok {

/**
 * Mode0Sequencer - Manages Mode 0 (Song Mode) sequencing
 *
 * Mode 0 is special - it runs at 1/16th speed and controls pattern
 * selection across all other modes. It also provides global parameters
 * like scale, velocity offsets, etc.
 */
class Mode0Sequencer {
public:
    explicit Mode0Sequencer(Song* song);

    /**
     * Start sequencing from beginning
     */
    void start();

    /**
     * Advance the song mode step
     * Call this every 16 normal steps (once per pattern loop)
     */
    void advanceStep();

    /**
     * Calculate loop length based on highest active step in Mode 0
     * Should be called after editing Mode 0 events
     */
    void calculateLoopLength();

    /**
     * Apply Mode 0 parameters from current step
     * Reads pattern selection, scale, velocity offsets from Mode 0 event
     * and applies them globally to modes 1-14
     */
    void applyParameters();

    /**
     * Parse a Mode 0 event and extract parameters for a specific mode
     * @param event Mode 0 event to parse
     * @param target_mode Which mode (1-14) to apply parameters to
     */
    void parseEvent(const Event& event, int target_mode);

    /**
     * Get current song mode step (0-15, advances every 16 normal steps)
     */
    int getCurrentStep() const { return song_mode_step_; }

    /**
     * Get song mode loop length (1-16)
     */
    int getLoopLength() const { return song_mode_loop_length_; }

    /**
     * Get pattern override for a specific mode
     * @param mode Mode number (1-14)
     * @return Pattern number (0-31) or -1 if no override
     */
    int getPatternOverride(int mode) const;

    /**
     * Get global scale root (0-11, C-B)
     */
    int getScaleRoot() const { return global_scale_root_; }

    /**
     * Get global scale type (0-N)
     */
    int getScaleType() const { return global_scale_type_; }

    /**
     * Get velocity offset for a specific mode
     * @param mode Mode number (0-14)
     * @return Velocity offset (-64 to +63)
     */
    int getVelocityOffset(int mode) const;

private:
    Song* song_;

    // Song mode (mode 0) - runs at 1/16th speed (each step = 1 full pattern)
    int song_mode_step_;         // Current step in Mode 0 (0-15)
    int song_mode_loop_length_;  // Loop length based on highest button pressed (1-16)

    // Mode 0 parameters: applied globally from Mode 0 events
    int global_scale_root_;      // 0-11 (C-B)
    int global_scale_type_;      // 0-N (Ionian, Dorian, etc.)
    int mode_velocity_offsets_[Song::NUM_MODES];  // Per-mode velocity offset (-64 to +63)
    int mode_pattern_overrides_[Song::NUM_MODES]; // Per-mode pattern override (0-31, or -1 for default)
};

} // namespace gruvbok
