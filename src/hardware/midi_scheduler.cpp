#include "midi_scheduler.h"

namespace gruvbok {

MidiScheduler::MidiScheduler(HardwareInterface* hardware)
    : hardware_(hardware)
    , audio_output_(nullptr)
    , use_internal_audio_(false)
    , use_external_midi_(true)  // Default to external MIDI
    , event_count_(0) {
    // Initialize all slots as inactive
    for (auto& event : event_buffer_) {
        event.active = false;
    }
}

void MidiScheduler::schedule(const std::vector<ScheduledMidiEvent>& events) {
    for (const auto& event : events) {
        schedule(event);
    }
}

void MidiScheduler::schedule(const ScheduledMidiEvent& event) {
    // Find a free slot
    int slot = findFreeSlot();
    if (slot < 0) {
        // Buffer full - drop event (could log warning in debug builds)
        return;
    }

    uint32_t current_time = hardware_->getMillis();
    uint32_t absolute_time = current_time + event.delta_ms;

    // Apply channel to MIDI message (for note on/off and CC)
    std::vector<uint8_t> data = event.data;
    if (!data.empty()) {
        uint8_t status = data[0] & 0xF0;  // Upper nibble = message type
        data[0] = status | (event.channel & 0x0F);  // Lower nibble = channel
    }

    // Store in buffer
    event_buffer_[slot].message = MidiMessage(data, absolute_time);
    event_buffer_[slot].absolute_time_ms = absolute_time;
    event_buffer_[slot].active = true;
    event_count_++;

    // Keep events sorted by time (insertion sort is fast for small arrays)
    // Only sort if we have more than one event
    if (event_count_ > 1) {
        sortEvents();
    }
}

void MidiScheduler::update() {
    uint32_t current_time = hardware_->getMillis();

    // Process events in order (buffer is kept sorted)
    for (int i = 0; i < MAX_QUEUED_EVENTS && event_count_ > 0; ++i) {
        if (!event_buffer_[i].active) {
            continue;
        }

        if (event_buffer_[i].absolute_time_ms <= current_time) {
            // Send to external MIDI
            if (use_external_midi_) {
                hardware_->sendMidiMessage(event_buffer_[i].message);
            }

            // Send to internal audio (FluidSynth)
            if (use_internal_audio_ && audio_output_ && audio_output_->isReady()) {
                audio_output_->sendMidiMessage(
                    event_buffer_[i].message.data.data(),
                    event_buffer_[i].message.data.size()
                );
            }

            // Mark slot as free
            event_buffer_[i].active = false;
            event_count_--;
        } else {
            // Events are sorted, so we can stop at first future event
            break;
        }
    }
}

void MidiScheduler::clear() {
    for (auto& event : event_buffer_) {
        event.active = false;
    }
    event_count_ = 0;
}

int MidiScheduler::findFreeSlot() {
    for (int i = 0; i < MAX_QUEUED_EVENTS; ++i) {
        if (!event_buffer_[i].active) {
            return i;
        }
    }
    return -1;  // Buffer full
}

void MidiScheduler::sortEvents() {
    // Simple insertion sort - efficient for small, nearly-sorted arrays
    // We only sort the active events by moving them to the front
    for (int i = 1; i < MAX_QUEUED_EVENTS; ++i) {
        if (!event_buffer_[i].active) {
            continue;
        }

        AbsoluteMidiEvent temp = event_buffer_[i];
        int j = i - 1;

        // Move earlier events that are later in time
        while (j >= 0 && event_buffer_[j].active &&
               event_buffer_[j].absolute_time_ms > temp.absolute_time_ms) {
            event_buffer_[j + 1] = event_buffer_[j];
            j--;
        }

        event_buffer_[j + 1] = temp;
    }
}

int MidiScheduler::getQueuedEventCount() const {
    return event_count_;
}

// ============================================================================
// Utility functions to create MIDI messages
// ============================================================================

ScheduledMidiEvent MidiScheduler::noteOn(uint8_t pitch, uint8_t velocity, uint8_t channel, uint32_t delta) {
    std::vector<uint8_t> data = {
        static_cast<uint8_t>(0x90 | (channel & 0x0F)),  // Note On + channel
        static_cast<uint8_t>(pitch & 0x7F),
        static_cast<uint8_t>(velocity & 0x7F)
    };
    return ScheduledMidiEvent(data, delta, channel);
}

ScheduledMidiEvent MidiScheduler::noteOff(uint8_t pitch, uint8_t channel, uint32_t delta) {
    std::vector<uint8_t> data = {
        static_cast<uint8_t>(0x80 | (channel & 0x0F)),  // Note Off + channel
        static_cast<uint8_t>(pitch & 0x7F),
        0x40  // Velocity (typically ignored)
    };
    return ScheduledMidiEvent(data, delta, channel);
}

ScheduledMidiEvent MidiScheduler::controlChange(uint8_t controller, uint8_t value, uint8_t channel, uint32_t delta) {
    std::vector<uint8_t> data = {
        static_cast<uint8_t>(0xB0 | (channel & 0x0F)),  // Control Change + channel
        static_cast<uint8_t>(controller & 0x7F),
        static_cast<uint8_t>(value & 0x7F)
    };
    return ScheduledMidiEvent(data, delta, channel);
}

ScheduledMidiEvent MidiScheduler::allNotesOff(uint8_t channel, uint32_t delta) {
    return controlChange(123, 0, channel, delta);  // CC 123 = All Notes Off
}

// ============================================================================
// MIDI Clock and Transport
// ============================================================================

void MidiScheduler::sendClock() {
    // MIDI Clock (0xF8) - System Real-Time message
    std::vector<uint8_t> data = {0xF8};
    MidiMessage msg(data, hardware_->getMillis());
    hardware_->sendMidiMessage(msg);
}

void MidiScheduler::sendStart() {
    // MIDI Start (0xFA) - System Real-Time message
    std::vector<uint8_t> data = {0xFA};
    MidiMessage msg(data, hardware_->getMillis());
    hardware_->sendMidiMessage(msg);
}

void MidiScheduler::sendStop() {
    // MIDI Stop (0xFC) - System Real-Time message
    std::vector<uint8_t> data = {0xFC};
    MidiMessage msg(data, hardware_->getMillis());
    hardware_->sendMidiMessage(msg);
}

void MidiScheduler::sendContinue() {
    // MIDI Continue (0xFB) - System Real-Time message
    std::vector<uint8_t> data = {0xFB};
    MidiMessage msg(data, hardware_->getMillis());
    hardware_->sendMidiMessage(msg);
}

// ============================================================================
// Audio Output Control
// ============================================================================

void MidiScheduler::setAudioOutput(AudioOutput* audio_output) {
    audio_output_ = audio_output;
}

void MidiScheduler::setUseInternalAudio(bool use_internal) {
    use_internal_audio_ = use_internal;
}

void MidiScheduler::setUseExternalMIDI(bool use_external) {
    use_external_midi_ = use_external;
}

} // namespace gruvbok
