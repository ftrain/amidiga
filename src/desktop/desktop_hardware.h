#pragma once

#include "../hardware/hardware_interface.h"
#include <memory>
#include <array>
#include <chrono>

// Forward declare RtMidiOut to avoid including RtMidi.h in header
class RtMidiOut;

namespace gruvbok {

/**
 * Desktop implementation of HardwareInterface
 * Uses keyboard for buttons and virtual MIDI output
 */
class DesktopHardware : public HardwareInterface {
public:
    DesktopHardware();
    ~DesktopHardware() override;

    bool init() override;
    void shutdown() override;

    bool readButton(int button) override;
    uint8_t readRotaryPot(int pot) override;
    uint8_t readSliderPot(int pot) override;

    void sendMidiMessage(const MidiMessage& msg) override;
    void setLED(bool on) override;
    uint32_t getMillis() override;

    void update() override;

    // Desktop-specific: simulate button press/release
    void simulateButton(int button, bool pressed);
    void simulateRotaryPot(int pot, uint8_t value);
    void simulateSliderPot(int pot, uint8_t value);

private:
    std::unique_ptr<RtMidiOut> midi_out_;
    std::array<bool, 16> buttons_;
    std::array<uint8_t, 4> rotary_pots_;
    std::array<uint8_t, 4> slider_pots_;
    std::chrono::steady_clock::time_point start_time_;
    bool led_state_;
    bool midi_initialized_;
};

} // namespace gruvbok
