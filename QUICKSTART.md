# GRUVBOK Quick Start

## TL;DR - Get It Running Fast

```bash
# 1. Install dependencies (one-time setup)
brew install cmake lua

# 2. Build
./build.sh

# 3. Run (from project root)
./build/bin/gruvbok
```

**Note:** RtMidi is bundled - no need to install it separately!

Press `Ctrl+C` to quit.

## What You'll See

GRUVBOK will start playing a demo drum pattern:
- Kick on beats 1, 2, 3, 4 (every quarter note)
- Snare on beats 2 and 4
- Hi-hat on every 8th note

You should see MIDI messages printed to the console:
```
[MIDI] 91 24 64    # Note on: Kick
[MIDI] 81 24 40    # Note off: Kick
[MIDI] 91 2a 46    # Note on: Hi-hat
...
```

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

✅ **Phase 1 Complete** - Core foundation working!

The system can:
- ✅ Store patterns in memory (Song/Mode/Pattern/Track/Event)
- ✅ Loop through Events at specified tempo
- ✅ Call Lua modes to generate MIDI
- ✅ Send MIDI out to macOS
- ✅ Play a test drum pattern

Not yet implemented:
- ⏳ Interactive keyboard input
- ⏳ Real-time pattern editing
- ⏳ Save/load songs
- ⏳ More musical modes
- ⏳ Teensy hardware port

See **DEVELOPMENT_ROADMAP.md** for the full plan.

## Have Fun!

GRUVBOK is designed to make music by pressing buttons and turning knobs. Right now it's playing a hardcoded pattern, but the foundation is solid.

Experiment, break things, and make noise! 🎵

Questions? Read **CLAUDE.md** or ask Claude for help.
