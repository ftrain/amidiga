#include "midi_clock_manager.h"

namespace gruvbok {

MidiClockManager::MidiClockManager(MidiScheduler* scheduler, HardwareInterface* hardware)
    : scheduler_(scheduler)
    , hardware_(hardware)
    , tempo_(120)
    , clock_start_time_(0)
    , clock_pulse_count_(0)
    , clock_interval_ms_(0.0) {
    calculateClockInterval();
}

void MidiClockManager::start() {
    clock_start_time_ = hardware_->getMillis();
    clock_pulse_count_ = 0;
    scheduler_->sendStart();
}

void MidiClockManager::stop() {
    scheduler_->sendStop();
}

void MidiClockManager::update() {
    uint32_t current_time = hardware_->getMillis();

    // Send MIDI clock messages at 24 PPQN using absolute timing to prevent drift
    // Calculate when the next clock pulse should occur
    uint32_t next_clock_time = clock_start_time_ + (uint32_t)(clock_pulse_count_ * clock_interval_ms_);

    // Send clock pulses for all that are due (can catch up if we fell behind)
    while (current_time >= next_clock_time) {
        sendClock();
        clock_pulse_count_++;
        next_clock_time = clock_start_time_ + (uint32_t)(clock_pulse_count_ * clock_interval_ms_);
    }
}

void MidiClockManager::setTempo(int bpm) {
    tempo_ = bpm;
    calculateClockInterval();
}

void MidiClockManager::calculateClockInterval() {
    // MIDI clock runs at 24 PPQN (pulses per quarter note)
    // Formula: (60000 / BPM) / 24 = ms per clock pulse
    // At 120 BPM: 60000 / 120 / 24 = 20.833333... ms per clock
    // Use double precision to prevent rounding errors
    clock_interval_ms_ = (60000.0 / static_cast<double>(tempo_)) / 24.0;
}

void MidiClockManager::sendClock() {
    scheduler_->sendClock();
}

} // namespace gruvbok
