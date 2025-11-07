#include "audio_output.h"
#include <iostream>

#ifdef HAVE_FLUIDSYNTH
#include <fluidsynth.h>
#endif

namespace gruvbok {

AudioOutput::AudioOutput()
    : settings_(nullptr)
    , synth_(nullptr)
    , audio_driver_(nullptr)
    , soundfont_id_(-1)
    , initialized_(false)
{
}

AudioOutput::~AudioOutput() {
#ifdef HAVE_FLUIDSYNTH
    if (audio_driver_) {
        delete_fluid_audio_driver(audio_driver_);
        audio_driver_ = nullptr;
    }
    if (synth_) {
        delete_fluid_synth(synth_);
        synth_ = nullptr;
    }
    if (settings_) {
        delete_fluid_settings(settings_);
        settings_ = nullptr;
    }
#endif
}

bool AudioOutput::init(int sample_rate) {
#ifdef HAVE_FLUIDSYNTH
    std::cout << "[AudioOutput] Initializing FluidSynth at " << sample_rate << " Hz...\n";

    // Create settings
    settings_ = new_fluid_settings();
    if (!settings_) {
        std::cerr << "[AudioOutput] Failed to create FluidSynth settings\n";
        return false;
    }

    // Configure audio settings
    fluid_settings_setnum(settings_, "synth.sample-rate", sample_rate);
    fluid_settings_setnum(settings_, "synth.gain", 0.5);  // Default gain
    fluid_settings_setint(settings_, "synth.polyphony", 256);  // Max voices
    fluid_settings_setint(settings_, "synth.midi-channels", 16);  // All MIDI channels

    // Create synthesizer
    synth_ = new_fluid_synth(settings_);
    if (!synth_) {
        std::cerr << "[AudioOutput] Failed to create FluidSynth synthesizer\n";
        delete_fluid_settings(settings_);
        settings_ = nullptr;
        return false;
    }

    // Create audio driver (automatically starts audio output)
    audio_driver_ = new_fluid_audio_driver(settings_, synth_);
    if (!audio_driver_) {
        std::cerr << "[AudioOutput] Failed to create FluidSynth audio driver\n";
        delete_fluid_synth(synth_);
        delete_fluid_settings(settings_);
        synth_ = nullptr;
        settings_ = nullptr;
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
    if (!synth_) {
        std::cerr << "[AudioOutput] Cannot load SoundFont: synth not initialized\n";
        return false;
    }

    std::cout << "[AudioOutput] Loading SoundFont: " << soundfont_path << "\n";

    // Unload previous SoundFont if any
    if (soundfont_id_ != -1) {
        fluid_synth_sfunload(synth_, soundfont_id_, 1);
    }

    // Load new SoundFont
    soundfont_id_ = fluid_synth_sfload(synth_, soundfont_path.c_str(), 1);
    if (soundfont_id_ == FLUID_FAILED) {
        std::cerr << "[AudioOutput] Failed to load SoundFont: " << soundfont_path << "\n";
        soundfont_id_ = -1;
        return false;
    }

    std::cout << "[AudioOutput] SoundFont loaded successfully (ID: " << soundfont_id_ << ")\n";
    return true;
#else
    std::cerr << "[AudioOutput] FluidSynth not available\n";
    return false;
#endif
}

void AudioOutput::sendMidiMessage(const uint8_t* data, size_t length) {
#ifdef HAVE_FLUIDSYNTH
    if (!synth_ || !initialized_ || soundfont_id_ == -1) {
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
                fluid_synth_noteoff(synth_, channel, data[1]);
            }
            break;

        case 0x90:  // Note On
            if (length >= 3) {
                if (data[2] == 0) {
                    // Velocity 0 = Note Off
                    fluid_synth_noteoff(synth_, channel, data[1]);
                } else {
                    fluid_synth_noteon(synth_, channel, data[1], data[2]);
                }
            }
            break;

        case 0xB0:  // Control Change
            if (length >= 3) {
                fluid_synth_cc(synth_, channel, data[1], data[2]);
            }
            break;

        case 0xC0:  // Program Change
            if (length >= 2) {
                fluid_synth_program_change(synth_, channel, data[1]);
            }
            break;

        case 0xE0:  // Pitch Bend
            if (length >= 3) {
                int value = (data[2] << 7) | data[1];
                fluid_synth_pitch_bend(synth_, channel, value);
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
    if (settings_) {
        double gain = 0.0;
        fluid_settings_getnum(settings_, "synth.gain", &gain);
        return static_cast<float>(gain);
    }
#endif
    return 0.0f;
}

void AudioOutput::setGain(float gain) {
#ifdef HAVE_FLUIDSYNTH
    if (synth_) {
        fluid_synth_set_gain(synth_, gain);
        std::cout << "[AudioOutput] Gain set to " << gain << "\n";
    }
#endif
}

} // namespace gruvbok
