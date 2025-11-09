#include "audio_output.h"
#include <iostream>

#ifdef HAVE_FLUIDSYNTH
#include <fluidsynth.h>
#endif

namespace gruvbok {

// FluidSynth implementation details (hidden from header)
struct FluidSynthImpl {
#ifdef HAVE_FLUIDSYNTH
    fluid_settings_t* settings = nullptr;
    fluid_synth_t* synth = nullptr;
    fluid_audio_driver_t* audio_driver = nullptr;
    int soundfont_id = -1;
#endif
};

AudioOutput::AudioOutput()
    : impl_(new FluidSynthImpl())
    , initialized_(false)
{
}

AudioOutput::~AudioOutput() {
#ifdef HAVE_FLUIDSYNTH
    if (impl_) {
        if (impl_->audio_driver) {
            delete_fluid_audio_driver(impl_->audio_driver);
        }
        if (impl_->synth) {
            delete_fluid_synth(impl_->synth);
        }
        if (impl_->settings) {
            delete_fluid_settings(impl_->settings);
        }
        delete impl_;
    }
#else
    delete impl_;
#endif
}

bool AudioOutput::init(int sample_rate) {
#ifdef HAVE_FLUIDSYNTH
    std::cout << "[AudioOutput] Initializing FluidSynth at " << sample_rate << " Hz...\n";

    // Create settings
    impl_->settings = new_fluid_settings();
    if (!impl_->settings) {
        std::cerr << "[AudioOutput] Failed to create FluidSynth settings\n";
        return false;
    }

    // Configure audio settings
    fluid_settings_setnum(impl_->settings, "synth.sample-rate", sample_rate);
    fluid_settings_setnum(impl_->settings, "synth.gain", 0.5);  // Default gain
    fluid_settings_setint(impl_->settings, "synth.polyphony", 256);  // Max voices
    fluid_settings_setint(impl_->settings, "synth.midi-channels", 16);  // All MIDI channels

    // Create synthesizer
    impl_->synth = new_fluid_synth(impl_->settings);
    if (!impl_->synth) {
        std::cerr << "[AudioOutput] Failed to create FluidSynth synthesizer\n";
        delete_fluid_settings(impl_->settings);
        impl_->settings = nullptr;
        return false;
    }

    // Create audio driver (automatically starts audio output)
    impl_->audio_driver = new_fluid_audio_driver(impl_->settings, impl_->synth);
    if (!impl_->audio_driver) {
        std::cerr << "[AudioOutput] Failed to create FluidSynth audio driver\n";
        delete_fluid_synth(impl_->synth);
        delete_fluid_settings(impl_->settings);
        impl_->synth = nullptr;
        impl_->settings = nullptr;
        return false;
    }

    initialized_ = true;
    std::cout << "[AudioOutput] FluidSynth initialized successfully\n";
    return true;
#else
    std::cerr << "[AudioOutput] FluidSynth not available (compiled without HAVE_FLUIDSYNTH)\n";
    return false;
#endif
}

bool AudioOutput::loadSoundFont(const std::string& soundfont_path) {
#ifdef HAVE_FLUIDSYNTH
    if (!impl_->synth) {
        std::cerr << "[AudioOutput] Cannot load SoundFont: synth not initialized\n";
        return false;
    }

    std::cout << "[AudioOutput] Loading SoundFont: " << soundfont_path << "\n";

    // Unload previous SoundFont if any
    if (impl_->soundfont_id != -1) {
        fluid_synth_sfunload(impl_->synth, impl_->soundfont_id, 1);
    }

    // Load new SoundFont
    impl_->soundfont_id = fluid_synth_sfload(impl_->synth, soundfont_path.c_str(), 1);
    if (impl_->soundfont_id == FLUID_FAILED) {
        std::cerr << "[AudioOutput] Failed to load SoundFont: " << soundfont_path << "\n";
        impl_->soundfont_id = -1;
        return false;
    }

    std::cout << "[AudioOutput] SoundFont loaded successfully (ID: " << impl_->soundfont_id << ")\n";

    // Set up default instruments for GRUVBOK modes
    // Mode 1 → Channel 0, Mode 2 → Channel 1, ..., Mode 10 → Channel 9 (GM drums)
    // Note: FluidSynth channels are 0-indexed (0-15)

    // Mode 10 → Channel 9 (GM Percussion on channel 10 in user-facing terms)
    fluid_synth_bank_select(impl_->synth, 9, 128);  // Bank 128 = GM Percussion
    fluid_synth_program_change(impl_->synth, 9, 0);  // Program 0 = Standard Kit
    std::cout << "[AudioOutput] Mode 10 (Channel 9/MIDI Ch 10): GM Drum Kit\n";

    // Set sensible defaults for other modes (will be overridden by Program Change)
    fluid_synth_program_change(impl_->synth, 0, 0);   // Mode 1 → Ch 0: Acoustic Grand Piano
    fluid_synth_program_change(impl_->synth, 1, 33);  // Mode 2 → Ch 1: Electric Bass
    fluid_synth_program_change(impl_->synth, 2, 48);  // Mode 3 → Ch 2: String Ensemble
    fluid_synth_program_change(impl_->synth, 3, 81);  // Mode 4 → Ch 3: Sawtooth Lead
    std::cout << "[AudioOutput] Default instruments set for modes 1-4\n";

    return true;
#else
    std::cerr << "[AudioOutput] FluidSynth not available\n";
    return false;
#endif
}

void AudioOutput::sendMidiMessage(const uint8_t* data, size_t length) {
#ifdef HAVE_FLUIDSYNTH
    if (!impl_->synth || !initialized_ || impl_->soundfont_id == -1) {
        return;  // Not ready
    }

    if (length < 1) {
        return;  // Invalid message
    }

    uint8_t status = data[0];
    uint8_t message_type = status & 0xF0;
    uint8_t channel = status & 0x0F;

    switch (message_type) {
        case 0x80:  // Note Off
            if (length >= 3) {
                fluid_synth_noteoff(impl_->synth, channel, data[1]);
            }
            break;

        case 0x90:  // Note On
            if (length >= 3) {
                if (data[2] == 0) {
                    // Velocity 0 = Note Off
                    fluid_synth_noteoff(impl_->synth, channel, data[1]);
                } else {
                    fluid_synth_noteon(impl_->synth, channel, data[1], data[2]);
                }
            }
            break;

        case 0xB0:  // Control Change
            if (length >= 3) {
                fluid_synth_cc(impl_->synth, channel, data[1], data[2]);
            }
            break;

        case 0xC0:  // Program Change
            if (length >= 2) {
                // For GM channel 10 (index 9), we need to select the drum bank
                if (channel == 9) {
                    fluid_synth_bank_select(impl_->synth, channel, 128);  // Bank 128 = GM Percussion
                }
                fluid_synth_program_change(impl_->synth, channel, data[1]);
            }
            break;

        case 0xE0:  // Pitch Bend
            if (length >= 3) {
                int value = (data[2] << 7) | data[1];
                fluid_synth_pitch_bend(impl_->synth, channel, value);
            }
            break;

        default:
            // Ignore other message types (system messages, etc.)
            break;
    }
#endif
}

float AudioOutput::getGain() const {
#ifdef HAVE_FLUIDSYNTH
    if (impl_ && impl_->settings) {
        double gain = 0.0;
        fluid_settings_getnum(impl_->settings, "synth.gain", &gain);
        return static_cast<float>(gain);
    }
#endif
    return 0.0f;
}

void AudioOutput::setGain(float gain) {
#ifdef HAVE_FLUIDSYNTH
    if (impl_ && impl_->synth) {
        fluid_synth_set_gain(impl_->synth, gain);
        std::cout << "[AudioOutput] Gain set to " << gain << "\n";
    }
#endif
}

} // namespace gruvbok
