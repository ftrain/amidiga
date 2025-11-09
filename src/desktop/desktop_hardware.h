#pragma once

#include "../hardware/hardware_base.h"
#include <memory>
#include <string>
#include <deque>

// Forward declare RtMidi classes to avoid including RtMidi.h in header
class RtMidiOut;
class RtMidiIn;

namespace gruvbok {

/**
 * @brief Desktop implementation of HardwareInterface
 *
 * Inherits common functionality from HardwareBase and adds:
 * - RtMidi MIDI output
 * - MIDI input (mirror mode)
 * - Logging functionality
 *
 * Uses keyboard/GUI for button simulation and virtual MIDI output.
 *
 * @note Inherits button/pot/LED state management from HardwareBase
 * @see HardwareBase for inherited functionality
 */
class DesktopHardware : public HardwareBase {
public:
    DesktopHardware();
    ~DesktopHardware() override;

    // HardwareInterface implementation
    bool init() override;
    void shutdown() override;
    void sendMidiMessage(const MidiMessage& msg) override;
    void update() override;

    // Note: Button/pot/LED methods inherited from HardwareBase
    // - readButton(), readRotaryPot(), readSliderPot()
    // - setLED(), getLED()
    // - getMillis()
    // - simulateButton(), simulateRotaryPot(), simulateSliderPot()

    // MIDI port management
    int getMidiPortCount();
    std::string getMidiPortName(int port);
    bool selectMidiPort(int port);
    int getCurrentMidiPort() const { return current_port_; }

    // MIDI input (mirror mode)
    int getMidiInputPortCount();
    std::string getMidiInputPortName(int port);
    bool selectMidiInputPort(int port);
    int getCurrentMidiInputPort() const { return current_input_port_; }
    bool isMirrorModeEnabled() const { return mirror_mode_enabled_; }
    void setMirrorMode(bool enabled);

    // Logging
    void addLog(const std::string& message);
    const std::deque<std::string>& getLogMessages() const { return log_messages_; }
    void clearLog();

private:
    // Desktop-specific state
    std::unique_ptr<RtMidiOut> midi_out_;
    std::unique_ptr<RtMidiIn> midi_in_;
    bool midi_initialized_;
    int current_port_;
    int current_input_port_;
    bool mirror_mode_enabled_;
    std::deque<std::string> log_messages_;
    static constexpr size_t MAX_LOG_MESSAGES = 100;

    // MIDI input callback
    static void midiInputCallback(double deltatime, std::vector<unsigned char>* message, void* userData);
};

} // namespace gruvbok
