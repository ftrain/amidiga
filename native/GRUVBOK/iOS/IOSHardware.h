#pragma once

#include "../../src/hardware/hardware_interface.h"
#include <array>
#include <memory>
#include <deque>
#include <string>
#include <CoreMIDI/CoreMIDI.h>

// Forward declarations
#ifdef __OBJC__
@class AVAudioEngine;
@class AVAudioUnitSampler;
@class UIImpactFeedbackGenerator;
#else
typedef struct objc_object AVAudioEngine;
typedef struct objc_object AVAudioUnitSampler;
typedef struct objc_object UIImpactFeedbackGenerator;
#endif

namespace gruvbok {

/**
 * Native iOS implementation using CoreMIDI and AVAudioEngine
 * Optimized for touch input with haptic feedback
 */
class IOSHardware : public HardwareInterface {
public:
    IOSHardware();
    ~IOSHardware() override;

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

    // Audio control (iOS prioritizes internal audio)
    bool initAudio(const std::string& soundfont_path = "");
    void setUseInternalAudio(bool use_internal) { use_internal_audio_ = use_internal; }
    void setUseExternalMIDI(bool use_external) { use_external_midi_ = use_external; }
    bool isAudioReady() const { return audio_initialized_; }
    void setAudioGain(float gain);

    // Logging
    void addLog(const std::string& message);
    const std::deque<std::string>& getLogMessages() const { return log_messages_; }
    void clearLog();

    // MIDI port info (for external hardware)
    int getMidiOutputCount();
    std::string getMidiOutputName(int index);
    bool selectMidiOutput(int index);

private:
    // MIDI (optional on iOS - most users use internal audio)
    MIDIClientRef midi_client_;
    MIDIPortRef midi_output_port_;
    int current_midi_output_;
    bool midi_initialized_;
    bool use_external_midi_;

    // Audio (Objective-C++ objects)
    AVAudioEngine* audio_engine_;
    AVAudioUnitSampler* sampler_;
    bool audio_initialized_;
    bool use_internal_audio_;

    // Haptic feedback
    UIImpactFeedbackGenerator* haptic_generator_;

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
    void triggerHaptic();
};

} // namespace gruvbok
