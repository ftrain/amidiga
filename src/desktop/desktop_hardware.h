#pragma once

#include "../hardware/hardware_interface.h"
#include <memory>
#include <array>
#include <chrono>
#include <string>
#include <vector>
#include <deque>

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

    // MIDI port management
    int getMidiPortCount();
    std::string getMidiPortName(int port);
    bool selectMidiPort(int port);
    int getCurrentMidiPort() const { return current_port_; }

    // Logging
    void addLog(const std::string& message);
    const std::deque<std::string>& getLogMessages() const { return log_messages_; }
    void clearLog();

private:
    std::unique_ptr<RtMidiOut> midi_out_;
    std::array<bool, 16> buttons_;
    std::array<uint8_t, 4> rotary_pots_;
    std::array<uint8_t, 4> slider_pots_;
    std::chrono::steady_clock::time_point start_time_;
    bool led_state_;
    bool midi_initialized_;
    int current_port_;
    std::deque<std::string> log_messages_;
    static constexpr size_t MAX_LOG_MESSAGES = 100;
};

} // namespace gruvbok
