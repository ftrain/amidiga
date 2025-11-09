#pragma once

#include "../hardware/hardware_interface.h"
#include "../hardware/midi_scheduler.h"
#include <cstdint>

namespace gruvbok {

/**
 * MIDI Clock Manager - Manages MIDI clock output at 24 PPQN
 *
 * Handles MIDI clock timing, start/stop messages, and ensures accurate
 * timing using absolute timestamps to prevent drift.
 */
class MidiClockManager {
public:
    explicit MidiClockManager(MidiScheduler* scheduler, HardwareInterface* hardware);

    /**
     * Start MIDI clock from beginning
     * Sends MIDI Start message and resets clock pulse count
     */
    void start();

    /**
     * Stop MIDI clock
     * Sends MIDI Stop message
     */
    void stop();

    /**
     * Update clock - call frequently in main loop
     * Sends clock pulses at 24 PPQN based on current tempo
     */
    void update();

    /**
     * Set tempo and recalculate clock interval
     * @param bpm Beats per minute (1-1000)
     */
    void setTempo(int bpm);

    /**
     * Get current tempo
     */
    int getTempo() const { return tempo_; }

private:
    MidiScheduler* scheduler_;
    HardwareInterface* hardware_;
    int tempo_;  // BPM

    // MIDI clock tracking (24 PPQN) - use absolute timing to prevent drift
    uint32_t clock_start_time_;     // When playback started (absolute time)
    uint32_t clock_pulse_count_;    // Number of clock pulses sent
    double clock_interval_ms_;      // Interval between clock pulses (float for precision)

    void calculateClockInterval();
    void sendClock();
};

} // namespace gruvbok
