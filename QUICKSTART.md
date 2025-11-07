# GRUVBOK Quick Start

## Get Running in 5 Minutes

```bash
# 1. Install dependencies (one-time setup)
brew install cmake lua sdl2    # macOS
# or
apt install cmake lua5.4 libsdl2-dev  # Linux

# 2. Build
cmake -B build && cmake --build build

# 3. Run the GUI simulator
bin/gruvbok
```

**Note:** RtMidi and Dear ImGui are bundled - you only need cmake, lua, and sdl2!

## What You'll See

GRUVBOK opens a **GUI window** showing a hardware simulator:

**Top Section - Status:**
- Current playback status, tempo, mode, pattern, track, step

**Global Controls (R1-R4):**
- R1: Mode selector (0-14)
- R2: Tempo (60-240 BPM)
- R3: Pattern selector (0-31)
- R4: Track selector (0-7)

**Step Buttons (B1-B16):**
- 16 checkboxes representing hardware buttons
- Toggle beats on/off for the current track
- Click to program steps

**Slider Pots (S1-S4):**
- 4 vertical sliders (mode-specific labels shown in GUI)
- **Mode 1 (Drums):** S1=Velocity, S2=Note Length
- **Mode 2 (Acid):** S1=Pitch, S2=Length, S3=Portamento, S4=Filter
- **Mode 3 (Chords):** S1=Root Note, S2=Chord Type, S3=Velocity, S4=Length

**Pattern Grid:**
- Visual representation of the current track
- Green = active step
- Red = currently playing step
- Click to toggle steps on/off

**Create patterns by:**
1. Click step buttons (B1-B16) to toggle events on/off
2. Move sliders (S1-S4) before clicking buttons to set parameters
3. Change Mode/Pattern/Track with rotary knobs (R1-R4)
4. Save your song with the "Save Song" button (saves to `/tmp/gruvbok_song_*.json`)

## Hearing Sound

GRUVBOK sends MIDI but doesn't make sound itself. You need a MIDI synthesizer:

### Option 1: Virtual MIDI (IAC Driver)
1. Open **Audio MIDI Setup** app
2. **Window → Show MIDI Studio**
3. Double-click **IAC Driver**
4. Check "**Device is online**"
5. GRUVBOK will send MIDI to this virtual port

### Option 2: Software Synth (Hear Actual Sounds)

```bash
# Install FluidSynth
brew install fluidsynth

# Download soundfont
curl -O https://musical-artifacts.com/artifacts/1336/GeneralUser_GS_v1.471.sf2

# Run synth (in another terminal)
fluidsynth -a coreaudio GeneralUser_GS_v1.471.sf2

# Now run GRUVBOK - you'll hear drums!
./build/bin/gruvbok
```

### Option 3: Use a DAW
Connect GRUVBOK's MIDI output to Logic Pro, Ableton Live, GarageBand, etc.

## Project Structure

```
gruvbok/
├── src/
│   ├── core/           # Data model and engine
│   ├── hardware/       # MIDI and hardware abstraction
│   ├── lua_bridge/     # Lua integration
│   └── desktop/        # macOS desktop app
├── modes/              # Lua mode scripts
│   ├── 00_boot.lua     # Boot/save/load mode
│   └── 01_drums.lua    # Drum machine mode
├── build/              # Build output (created by ./build.sh)
└── BUILD.md            # Detailed build instructions
```

## Understanding GRUVBOK

GRUVBOK is an "always-playing" groovebox:

1. **Song** data structure contains all patterns
2. **Engine** loops through Events (16 steps per track)
3. **Lua modes** transform Events into MIDI notes
4. **Desktop hardware** sends MIDI to your Mac

Currently it just plays a hardcoded test pattern. Next steps:

1. **Modify the pattern** - Edit `src/desktop/main.cpp` around line 70
2. **Create new modes** - Copy `modes/TEMPLATE.lua`
3. **Add keyboard input** - Make it interactive!

## Next Steps

### Read the Docs
- **CLAUDE.md** - Complete technical architecture
- **BUILD.md** - Detailed build instructions and troubleshooting
- **PROJECT_STRUCTURE.md** - Directory layout
- **DEVELOPMENT_ROADMAP.md** - Implementation plan

### Modify the Test Pattern

Edit `src/desktop/main.cpp` to change the pattern:

```cpp
// Create a kick pattern on track 0
for (int step : {0, 4, 8, 12}) {  // Change these step numbers!
    Event& event = pattern0.getEvent(0, step);
    event.setSwitch(true);
    event.setPot(0, 100);  // Velocity (0-127)
    event.setPot(1, 50);   // Note length (ms)
}
```

Rebuild with `./build.sh` and run again.

### Create a New Mode

```bash
cp modes/TEMPLATE.lua modes/02_mymode.lua
# Edit the file
# Rebuild and run
```

### Explore Slash Commands

```
/architecture-check  - See what's implemented
/design-mode         - Brainstorm new mode ideas
/explain-mode        - Understand existing modes
```

## Troubleshooting

**Build fails with "Lua not found":**
```bash
brew install lua
./build.sh
```

**Build fails with "RtMidi not found":**
```bash
brew install rtmidi
./build.sh
```

**No MIDI output:**
- Enable IAC Driver (see "Hearing Sound" above)
- Install MIDI Monitor to see messages: https://www.snoize.com/MIDIMonitor/

**Program crashes immediately:**
Make sure you run from project root (not from `build/`):
```bash
pwd  # Should show .../gruvbok
./build/bin/gruvbok
```

## Status

✅ **Desktop Implementation Complete!**

**Working:**
- ✅ Full GUI with pattern grid visualization
- ✅ Real-time pattern editing (click to toggle steps)
- ✅ Save/Load songs (JSON format, `/tmp/gruvbok_song_*.json`)
- ✅ 15 musical modes (drums, acid, chords, + 11 experimental)
- ✅ Multi-timbral playback (all modes play simultaneously)
- ✅ Song Data Explorer window
- ✅ System log with MIDI event monitoring
- ✅ Full test suite (56/56 tests passing)

**Pending:**
- ⏳ Teensy 4.1 hardware testing (firmware ready, needs physical device)

See **CLAUDE.md** for complete status and **docs/TEENSY_DEPLOYMENT_GUIDE.md** for hardware deployment.

## Have Fun!

GRUVBOK is designed to make music by pressing buttons and turning knobs. Right now it's playing a hardcoded pattern, but the foundation is solid.

Experiment, break things, and make noise! 🎵

Questions? Read **CLAUDE.md** or ask Claude for help.
