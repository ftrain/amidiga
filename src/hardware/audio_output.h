#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H

#include <string>
#include <cstdint>

// Forward declare FluidSynth types to avoid header dependency
typedef struct _fluid_settings_t fluid_settings_t;
typedef struct _fluid_synth_t fluid_synth_t;
typedef struct _fluid_audio_driver_t fluid_audio_driver_t;

namespace gruvbok {

/**
 * @brief Audio output using FluidSynth for internal synthesis
 *
 * Wraps FluidSynth to provide internal audio rendering.
 * Supports loading SoundFonts and processing MIDI messages.
 */
class AudioOutput {
public:
    AudioOutput();
    ~AudioOutput();

    // Disable copy and move
    AudioOutput(const AudioOutput&) = delete;
    AudioOutput& operator=(const AudioOutput&) = delete;

    /**
     * @brief Initialize FluidSynth with audio driver
     * @param sample_rate Audio sample rate (default: 44100)
     * @return true if initialization succeeded
     */
    bool init(int sample_rate = 44100);

    /**
     * @brief Load a SoundFont file
     * @param soundfont_path Path to .sf2 file
     * @return true if loading succeeded
     */
    bool loadSoundFont(const std::string& soundfont_path);

    /**
     * @brief Send a MIDI message to FluidSynth
     * @param data MIDI message bytes
     * @param length Number of bytes
     */
    void sendMidiMessage(const uint8_t* data, size_t length);

    /**
     * @brief Check if FluidSynth is initialized and ready
     */
    bool isReady() const { return synth_ != nullptr; }

    /**
     * @brief Get the current gain (volume) setting
     * @return Gain value (0.0 to 10.0, default 0.2)
     */
    float getGain() const;

    /**
     * @brief Set the gain (volume)
     * @param gain Gain value (0.0 to 10.0)
     */
    void setGain(float gain);

private:
    fluid_settings_t* settings_;
    fluid_synth_t* synth_;
    fluid_audio_driver_t* audio_driver_;
    int soundfont_id_;
    bool initialized_;
};

} // namespace gruvbok

#endif // AUDIO_OUTPUT_H
