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
@class UIImpactFeedbackGenerator;
#else
typedef struct objc_object AVAudioEngine;
typedef struct objc_object AVAudioUnitSampler;
typedef struct objc_object UIImpactFeedbackGenerator;
#endif

namespace gruvbok {

/**
 * @brief Native iOS implementation using CoreMIDI and AVAudioEngine
 *
 * Inherits common functionality from HardwareBase and adds:
 * - CoreMIDI output (for external hardware)
 * - AVAudioEngine for internal synthesis (prioritized on iOS)
 * - Haptic feedback for touch interactions
 * - Logging functionality
 *
 * Optimized for touch input with haptic feedback.
 *
 * @note Inherits button/pot/LED state management from HardwareBase
 * @see HardwareBase for inherited functionality
 */
class IOSHardware : public HardwareBase {
public:
    IOSHardware();
    ~IOSHardware() override;

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
    // iOS-specific MIDI state (optional - most users use internal audio)
    MIDIClientRef midi_client_;
    MIDIPortRef midi_output_port_;
    int current_midi_output_;
    bool midi_initialized_;
    bool use_external_midi_;

    // iOS-specific audio state (Objective-C++ objects)
    AVAudioEngine* audio_engine_;
    AVAudioUnitSampler* sampler_;
    bool audio_initialized_;
    bool use_internal_audio_;

    // iOS-specific haptic feedback
    UIImpactFeedbackGenerator* haptic_generator_;

    // Timing (iOS-specific, overrides base implementation)
    uint64_t ios_start_time_;  // mach_absolute_time at init

    // Logging
    std::deque<std::string> log_messages_;
    static constexpr size_t MAX_LOG_MESSAGES = 100;

    void sendToCoreMIDI(const MidiMessage& msg);
    void sendToAudioEngine(const MidiMessage& msg);
    void triggerHaptic();
};

} // namespace gruvbok
