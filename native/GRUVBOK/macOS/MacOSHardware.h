#pragma once

#include "../../../../src/hardware/hardware_base.h"
#include <memory>
#include <deque>
#include <string>
#include <CoreMIDI/CoreMIDI.h>

// Forward declarations
#ifdef __OBJC__
@class AVAudioEngine;
@class AVAudioUnitSampler;
#else
typedef struct objc_object AVAudioEngine;
typedef struct objc_object AVAudioUnitSampler;
#endif

namespace gruvbok {

/**
 * @brief Native macOS implementation using CoreMIDI and AVAudioEngine
 *
 * Inherits common functionality from HardwareBase and adds:
 * - CoreMIDI output (virtual source + physical destinations)
 * - AVAudioEngine for internal synthesis
 * - Logging functionality
 *
 * @note Inherits button/pot/LED state management from HardwareBase
 * @see HardwareBase for inherited functionality
 */
class MacOSHardware : public HardwareBase {
public:
    MacOSHardware();
    ~MacOSHardware() override;

    // HardwareInterface implementation
    bool init() override;
    void shutdown() override;
    void sendMidiMessage(const MidiMessage& msg) override;
    void update() override;
    uint32_t getMillis() override;  // Override with mach_absolute_time

    // Note: Button/pot/LED/simulation methods inherited from HardwareBase
    // - readButton(), readRotaryPot(), readSliderPot()
    // - setLED(), getLED()
    // - simulateButton(), simulateRotaryPot(), simulateSliderPot()

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

    // MIDI port info
    int getMidiOutputCount();
    std::string getMidiOutputName(int index);
    bool selectMidiOutput(int index);

private:
    // macOS-specific MIDI state
    MIDIClientRef midi_client_;
    MIDIPortRef midi_output_port_;
    MIDIEndpointRef midi_virtual_source_;
    int current_midi_output_;
    bool midi_initialized_;
    bool use_external_midi_;

    // macOS-specific audio state (Objective-C++ objects)
    AVAudioEngine* audio_engine_;
    AVAudioUnitSampler* sampler_;
    bool audio_initialized_;
    bool use_internal_audio_;

    // Timing (macOS-specific, overrides base implementation)
    uint64_t macos_start_time_;  // mach_absolute_time at init

    // Logging
    std::deque<std::string> log_messages_;
    static constexpr size_t MAX_LOG_MESSAGES = 100;

    void sendToCoreMIDI(const MidiMessage& msg);
    void sendToAudioEngine(const MidiMessage& msg);
};

} // namespace gruvbok
