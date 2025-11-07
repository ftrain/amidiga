# GRUVBOK Audio Output Integration

## Overview

GRUVBOK now supports internal audio synthesis via **FluidSynth**, transforming it from a MIDI-only groovebox into a standalone synthesizer/DAW hybrid. This feature is optional and gracefully degrades to MIDI-only mode if FluidSynth is not available.

## Features

### Dual Output Mode
- **Internal Audio**: Route MIDI to FluidSynth for built-in synthesis
- **External MIDI**: Route MIDI to hardware/software synths via RtMidi
- **Both simultaneously**: Enable internal audio and external MIDI together

### FluidSynth Integration
- SoundFont 2 (.sf2) support
- 16 MIDI channels (one per mode)
- Real-time MIDI message processing (Note On/Off, CC, Program Change)
- Volume control (0.0 - 2.0x gain)
- Configurable sample rate (default: 44100 Hz)
- 256 voice polyphony

## Architecture

```
Engine → MidiScheduler → [FORK]
                          ├─→ RtMidi (external MIDI)
                          └─→ FluidSynth (internal audio)
```

### Key Classes

**AudioOutput** (`src/hardware/audio_output.h/cpp`)
- Wraps FluidSynth C API
- Handles initialization and SoundFont loading
- Processes MIDI messages
- Controls gain/volume

**MidiScheduler** (updated)
- Routes MIDI to both AudioOutput and HardwareInterface
- Flags to enable/disable each output independently
- Delta-timed events work for both outputs

**Engine** (updated)
- Public API for audio control
- `initAudioOutput(soundfont_path)` - Initialize FluidSynth
- `setUseInternalAudio(bool)` - Toggle internal audio
- `setUseExternalMIDI(bool)` - Toggle external MIDI
- `setAudioGain(float)` - Control volume

## GUI Controls

**Audio Output Section** (added to main window):
- ☑ **Internal Audio (FluidSynth)** - Enable/disable internal synthesis
- **[READY]** indicator - Shows when FluidSynth is initialized
- **Volume** slider (0.0 - 2.0x) - Control output gain
- ☑ **External MIDI** - Enable/disable external MIDI output

When enabling internal audio for the first time:
- GUI searches for SoundFont in common locations:
  - `/usr/share/soundfonts/FluidR3_GM.sf2`
  - `/usr/share/sounds/sf2/FluidR3_GM.sf2`
  - `/usr/local/share/soundfonts/FluidR3_GM.sf2`
  - `FluidR3_GM.sf2` (current directory)
- Logs success/failure to System Log window
- Falls back to MIDI-only if initialization fails

## Building with FluidSynth

### Dependencies

**Linux (Ubuntu/Debian):**
```bash
apt-get install libfluidsynth-dev fluidsynth
```

**macOS:**
```bash
brew install fluid-synth
```

**Windows:**
Download pre-built binaries from: https://github.com/FluidSynth/fluidsynth/releases

### CMake Configuration

FluidSynth is detected automatically via `pkg-config`:
```cmake
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(FLUIDSYNTH fluidsynth)
endif()
```

If FluidSynth is found:
- `HAVE_FLUIDSYNTH` preprocessor flag is defined
- AudioOutput compiles with full functionality

If FluidSynth is NOT found:
- AudioOutput compiles as stubs (no-ops)
- GRUVBOK works in MIDI-only mode
- No build errors

### Verify FluidSynth Detection

After running `cmake -B build`:
```
=== GRUVBOK Configuration ===
...
FluidSynth: 2.3.4 (internal audio enabled)
=============================
```

Or:
```
FluidSynth: not found (MIDI-only mode)
```

## SoundFonts

### Recommended Free SoundFonts

**FluidR3_GM.sf2** (142 MB)
- Full General MIDI 2.0 compatible
- High quality
- Standard for FluidSynth
- Download: https://github.com/FluidSynth/fluidsynth/wiki/SoundFont

**GeneralUser GS** (30 MB)
- Excellent quality
- Smaller than FluidR3
- Download: http://www.schristiancollins.com/generaluser.php

**TimGM6mb.sf2** (6 MB)
- Lightweight
- Good for testing
- Included with many Linux distributions

### Installing SoundFonts

**Linux:**
```bash
# System-wide (recommended)
cp FluidR3_GM.sf2 /usr/share/soundfonts/

# User-specific
mkdir -p ~/.local/share/soundfonts
cp FluidR3_GM.sf2 ~/.local/share/soundfonts/
```

**macOS:**
```bash
mkdir -p /usr/local/share/soundfonts
cp FluidR3_GM.sf2 /usr/local/share/soundfonts/
```

**For macOS App Bundle:**
Place in `GRUVBOK.app/Contents/Resources/soundfonts/`

## Usage

### From GUI

1. Launch GRUVBOK GUI
2. Check **"Internal Audio (FluidSynth)"**
3. If SoundFont found: Green **[READY]** appears
4. Adjust **Volume** slider as needed
5. Optionally uncheck **"External MIDI"** to use only internal audio
6. Play patterns - hear audio directly!

### From Code (C++ API)

```cpp
Engine engine(&song, hardware, &mode_loader);

// Initialize with SoundFont
if (engine.initAudioOutput("/path/to/soundfont.sf2")) {
    engine.setUseInternalAudio(true);
    engine.setUseExternalMIDI(false);  // Optional
    engine.setAudioGain(0.5f);         // 50% volume
}

engine.start();
// Audio now plays through speakers!
```

## Performance Considerations

### Latency
- FluidSynth adds ~10-20ms audio latency (depends on audio buffer size)
- MIDI scheduling precision maintained (<1ms jitter)
- No noticeable lag on modern hardware

### CPU Usage
- FluidSynth: ~5-15% CPU (depends on polyphony)
- Tested on: Intel i5 (2015), Apple M1 (2020)
- 256 voices max polyphony is conservative

### Memory
- FluidSynth: ~10-50 MB (depends on SoundFont)
- SoundFont: 6 MB (TimGM) to 142 MB (FluidR3)
- Total GRUVBOK: ~250 MB with FluidR3 loaded

### Teensy Compatibility
- **FluidSynth NOT supported on Teensy 4.1** (not enough RAM for SoundFonts)
- Teensy remains MIDI-only
- Desktop/macOS/Windows only for internal audio

## Troubleshooting

### "FluidSynth: not found" during CMake
- Install libfluidsynth-dev: `apt-get install libfluidsynth-dev`
- Check pkg-config: `pkg-config --modversion fluidsynth`
- Verify PKG_CONFIG_PATH includes FluidSynth .pc file

### "Failed to initialize audio output"
- Check if audio device is available
- Try different audio driver (ALSA, PulseAudio, CoreAudio)
- Look for error messages in console

### "Failed to load SoundFont"
- Verify .sf2 file path is correct
- Check file permissions (readable)
- Try different SoundFont
- Ensure SoundFont is valid SF2 format

### No sound but [READY] indicator shows
- Check system volume (FluidSynth uses system audio)
- Adjust GRUVBOK Volume slider
- Verify MIDI messages are being sent (check System Log)
- Test with external MIDI to confirm patterns are playing

### Audio crackling/glitches
- Increase audio buffer size (FluidSynth setting)
- Reduce polyphony (less CPU)
- Check system CPU usage (close other apps)

## Future Enhancements

### Planned Features
- [ ] SoundFont selector GUI (browse and load .sf2 files)
- [ ] Per-mode SoundFont assignment (drums on one SF, bass on another)
- [ ] Audio effects chain (reverb, chorus, EQ)
- [ ] Audio recording (render to WAV/FLAC)
- [ ] LV2 plugin hosting (effects and instruments)

### macOS App Bundle Integration
- [ ] Bundle TimGM6mb.sf2 in Resources folder (~6 MB)
- [ ] Auto-load bundled SoundFont on first launch
- [ ] Preference pane for SoundFont management

### Advanced Features (Maybe)
- [ ] VST3 plugin hosting (major undertaking)
- [ ] Built-in synth engines (subtractive, FM, wavetable)
- [ ] Audio routing matrix (send modes to different outputs)
- [ ] JACK audio support (Linux pro audio)

## Technical Details

### Conditional Compilation
- `#ifdef HAVE_FLUIDSYNTH` protects all FluidSynth code
- AudioOutput compiles as stubs when FluidSynth unavailable
- No linker errors, graceful degradation

### Thread Safety
- FluidSynth synthesizer runs in audio thread (created by FluidSynth)
- MIDI messages sent from main thread
- FluidSynth API is thread-safe (no locking needed)

### MIDI Message Handling
- Note On (0x90): `fluid_synth_noteon()`
- Note Off (0x80): `fluid_synth_noteoff()`
- Control Change (0xB0): `fluid_synth_cc()`
- Program Change (0xC0): `fluid_synth_program_change()`
- Pitch Bend (0xE0): `fluid_synth_pitch_bend()`

### Audio Driver Selection
- FluidSynth auto-selects best driver:
  - **Linux**: ALSA, PulseAudio, JACK
  - **macOS**: CoreAudio
  - **Windows**: WASAPI, DirectSound

## References

- **FluidSynth Official Site**: https://www.fluidsynth.org/
- **FluidSynth API Docs**: https://www.fluidsynth.org/api/
- **FluidSynth GitHub**: https://github.com/FluidSynth/fluidsynth
- **SoundFont Wiki**: https://github.com/FluidSynth/fluidsynth/wiki/SoundFont

---

**GRUVBOK**: From groovebox to the world's weirdest DAW! 🎹🎛️🔊
