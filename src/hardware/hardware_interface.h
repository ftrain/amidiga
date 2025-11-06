#pragma once

#include <cstdint>
#include <vector>

namespace gruvbok {

/**
 * MIDI message structure
 */
struct MidiMessage {
    std::vector<uint8_t> data;
    uint32_t timestamp_ms;  // Absolute timestamp in milliseconds

    MidiMessage() : timestamp_ms(0) {}
    MidiMessage(const std::vector<uint8_t>& msg_data, uint32_t time)
        : data(msg_data), timestamp_ms(time) {}
};

/**
 * Hardware abstraction interface
 * Implemented differently for desktop and Teensy
 */
class HardwareInterface {
public:
    virtual ~HardwareInterface() = default;

    // Initialization
    virtual bool init() = 0;
    virtual void shutdown() = 0;

    // Button input (B1-B16)
    virtual bool readButton(int button) = 0;  // button: 0-15, returns true if pressed

    // Pot input (R1-R4 for rotary, S1-S4 for sliders)
    // Returns value 0-127 (MIDI range)
    virtual uint8_t readRotaryPot(int pot) = 0;  // pot: 0-3 (R1-R4)
    virtual uint8_t readSliderPot(int pot) = 0;  // pot: 0-3 (S1-S4)

    // MIDI output
    virtual void sendMidiMessage(const MidiMessage& msg) = 0;

    // LED control
    virtual void setLED(bool on) = 0;
    virtual bool getLED() const = 0;  // Get current LED state

    // Timing
    virtual uint32_t getMillis() = 0;  // Milliseconds since start

    // Update (called in main loop)
    virtual void update() = 0;
};

} // namespace gruvbok
