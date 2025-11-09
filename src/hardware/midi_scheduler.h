#pragma once

#include "hardware_interface.h"
#include "audio_output.h"
#include <array>
#include <vector>
#include <algorithm>

namespace gruvbok {

/**
 * Scheduled MIDI event (relative timing)
 */
struct ScheduledMidiEvent {
    std::vector<uint8_t> data;
    uint32_t delta_ms;  // Milliseconds from now
    uint8_t channel;    // MIDI channel 0-15

    ScheduledMidiEvent(const std::vector<uint8_t>& msg_data, uint32_t delta, uint8_t chan)
        : data(msg_data), delta_ms(delta), channel(chan) {}
};

/**
 * Internal event with absolute timing (for static array)
 */
struct AbsoluteMidiEvent {
    MidiMessage message;
    uint32_t absolute_time_ms;
    bool active;  // Whether this slot is in use

    AbsoluteMidiEvent() : absolute_time_ms(0), active(false) {}

    bool operator<(const AbsoluteMidiEvent& other) const {
        return absolute_time_ms < other.absolute_time_ms;
    }
};

/**
 * MIDI Scheduler handles delta-timed MIDI events
 * Converts relative timing to absolute and sends at precise times
 * Supports routing to external MIDI and/or internal audio (FluidSynth)
 *
 * IMPLEMENTATION NOTE:
 * Uses a static array instead of std::priority_queue to avoid dynamic
 * allocations in real-time code. Typical usage is 1-16 events per step,
 * so a fixed-size buffer of 64 events is more than sufficient and provides
 * predictable, allocation-free performance suitable for embedded systems.
 */
class MidiScheduler {
public:
    explicit MidiScheduler(HardwareInterface* hardware);

    // Schedule MIDI events (relative timing)
    void schedule(const std::vector<ScheduledMidiEvent>& events);
    void schedule(const ScheduledMidiEvent& event);

    // Update - call frequently to send scheduled events
    void update();

    // Clear all scheduled events
    void clear();

    // Audio output control
    void setAudioOutput(AudioOutput* audio_output);
    void setUseInternalAudio(bool use_internal);
    void setUseExternalMIDI(bool use_external);
    bool isUsingInternalAudio() const { return use_internal_audio_; }
    bool isUsingExternalMIDI() const { return use_external_midi_; }

    // Utility: Create common MIDI messages
    static ScheduledMidiEvent noteOn(uint8_t pitch, uint8_t velocity, uint8_t channel, uint32_t delta = 0);
    static ScheduledMidiEvent noteOff(uint8_t pitch, uint8_t channel, uint32_t delta = 0);
    static ScheduledMidiEvent controlChange(uint8_t controller, uint8_t value, uint8_t channel, uint32_t delta = 0);
    static ScheduledMidiEvent allNotesOff(uint8_t channel, uint32_t delta = 0);

    // MIDI Clock and transport
    void sendClock();       // Send MIDI clock message (0xF8)
    void sendStart();       // Send MIDI start message (0xFA)
    void sendStop();        // Send MIDI stop message (0xFC)
    void sendContinue();    // Send MIDI continue message (0xFB)

    // Statistics (for debugging/monitoring)
    int getQueuedEventCount() const;
    int getMaxQueueCapacity() const { return MAX_QUEUED_EVENTS; }

private:
    HardwareInterface* hardware_;
    AudioOutput* audio_output_;
    bool use_internal_audio_;
    bool use_external_midi_;

    // Static event buffer (no dynamic allocation)
    static constexpr int MAX_QUEUED_EVENTS = 64;
    std::array<AbsoluteMidiEvent, MAX_QUEUED_EVENTS> event_buffer_;
    int event_count_;

    // Helper to find next free slot
    int findFreeSlot();

    // Helper to sort active events by time
    void sortEvents();
};

} // namespace gruvbok
