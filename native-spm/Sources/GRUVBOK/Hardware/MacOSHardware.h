#pragma once

#include "hardware/hardware_interface.h"
#include "hardware/audio_output.h"
#include <array>
#include <memory>
#include <deque>
#include <string>
#include <CoreMIDI/CoreMIDI.h>

namespace gruvbok {

/**
 * Native macOS implementation using CoreMIDI and AVAudioEngine
 */
class MacOSHardware : public HardwareInterface {
public:
    MacOSHardware();
    ~MacOSHardware() override;

    bool init() override;
    void shutdown() override;

    bool readButton(int button) override;
    uint8_t readRotaryPot(int pot) override;
    uint8_t readSliderPot(int pot) override;

    void sendMidiMessage(const MidiMessage& msg) override;
    void setLED(bool on) override;
    bool getLED() const override { return led_state_; }
    uint32_t getMillis() override;

    void update() override;

    // Simulation methods (called from SwiftUI)
    void simulateButton(int button, bool pressed);
    void simulateRotaryPot(int pot, uint8_t value);
    void simulateSliderPot(int pot, uint8_t value);

    // Audio control
    bool initAudio(const std::string& soundfont_path = "");
    void setUseInternalAudio(bool use_internal) { use_internal_audio_ = use_internal; }
    void setUseExternalMIDI(bool use_external) { use_external_midi_ = use_external; }
    bool isAudioReady() const { return audio_initialized_; }
    void setAudioGain(float gain);

    // Logging
    void addLog(const std::string& message);
    const std::deque<std::string>& getLogMessages() const { return log_messages_; }
    void clearLog();

    // MIDI output port info
    int getMidiOutputCount();
    std::string getMidiOutputName(int index);
    bool selectMidiOutput(int index);

    // MIDI input port info (mirror mode)
    int getMidiInputCount();
    std::string getMidiInputName(int index);
    bool selectMidiInput(int index);
    int getCurrentMidiInput() const { return current_midi_input_; }
    bool isMirrorModeEnabled() const { return mirror_mode_enabled_; }
    void setMirrorMode(bool enabled);

private:
    // MIDI output
    MIDIClientRef midi_client_;
    MIDIPortRef midi_output_port_;
    MIDIEndpointRef midi_virtual_source_;
    int current_midi_output_;
    bool midi_initialized_;
    bool use_external_midi_;

    // MIDI input (mirror mode)
    MIDIPortRef midi_input_port_;
    int current_midi_input_;
    bool mirror_mode_enabled_;

    // Audio (FluidSynth)
    std::unique_ptr<AudioOutput> audio_output_;
    bool audio_initialized_;
    bool use_internal_audio_;

    // Hardware state
    std::array<bool, 16> buttons_;
    std::array<uint8_t, 4> rotary_pots_;
    std::array<uint8_t, 4> slider_pots_;
    uint64_t start_time_;
    bool led_state_;

    // Logging
    std::deque<std::string> log_messages_;
    static constexpr size_t MAX_LOG_MESSAGES = 100;

    void sendToCoreMIDI(const MidiMessage& msg);
    void sendToAudioEngine(const MidiMessage& msg);
};

} // namespace gruvbok
