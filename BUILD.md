# Building GRUVBOK on macOS

This guide explains how to build and run GRUVBOK on your Mac.

## Prerequisites

You'll need the following tools and libraries installed:

### 1. Install Homebrew

If you don't have Homebrew, install it:

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### 2. Install Dependencies

```bash
# Install build tools
brew install cmake

# Install Lua
brew install lua
```

**Note:** RtMidi is bundled in `external/rtmidi/` - you don't need to install it!

### 3. Verify Installation

```bash
lua -v          # Should show Lua 5.4.x
cmake --version # Should show CMake 3.15 or higher
```

## Building

### 1. Clone or Navigate to the Project

```bash
cd /path/to/gruvbok
```

### 2. Create Build Directory

```bash
mkdir build
cd build
```

### 3. Configure with CMake

```bash
cmake ..
```

You should see output like:
```
-- Found Lua: /opt/homebrew/lib/liblua.dylib
-- Found RtMidi: /opt/homebrew/lib/librtmidi.dylib
-- Configuring done
-- Generating done
```

### 4. Build

```bash
make -j$(sysctl -n hw.ncpu)
```

This will compile all source files. The executable will be in `build/bin/gruvbok`.

### 5. Run

From the **project root directory** (not from build/), run:

```bash
./build/bin/gruvbok
```

**Important:** You must run from the project root so that the program can find the `modes/` directory with Lua scripts.

## Expected Output

When you run GRUVBOK, you should see:

```
=== GRUVBOK Desktop ===
Initializing...
Available MIDI ports:
  0: IAC Driver Bus 1
  ...
Opening MIDI port 0
Desktop hardware initialized
Loaded mode 0 from modes/00_boot.lua
Loaded mode 1 from modes/01_drums.lua
Loaded 2 modes from modes

Creating test pattern...

=== GRUVBOK Desktop ===

Commands:
  Space   - Start/Stop playback
  ...

Running main loop (press Ctrl+C to quit)...
[Mode:1 Pat:0 Trk:0 Step:0 Tempo:120bpm PLAYING]
[MIDI] 91 24 64
[MIDI] 81 24 40
...
```

You should hear drum beats if you have a MIDI synthesizer connected!

## Troubleshooting

### CMake can't find Lua

If CMake says "Could not find Lua":

```bash
# Check where Homebrew installed Lua
brew info lua

# Set the path manually
cmake .. -DLUA_INCLUDE_DIR=/opt/homebrew/include/lua -DLUA_LIBRARIES=/opt/homebrew/lib/liblua.dylib
```

### Build errors with RtMidi

RtMidi is bundled in the project, so you shouldn't see these errors. If you do:

```bash
# Verify the bundled library exists
ls external/rtmidi/RtMidi.h
ls external/rtmidi/RtMidi.cpp

# If missing, re-clone the repository
```

### No MIDI output

If you don't see any MIDI ports:

1. Open **Audio MIDI Setup** (in `/Applications/Utilities/`)
2. Go to **Window → Show MIDI Studio**
3. Double-click **IAC Driver**
4. Check **Device is online**
5. Make sure there's at least one port (e.g., "Bus 1")

### Program crashes or doesn't run

Make sure you're running from the project root:

```bash
# Wrong (from build directory)
cd build
./bin/gruvbok  # Won't find modes/ directory!

# Correct (from project root)
cd /path/to/gruvbok
./build/bin/gruvbok
```

## Testing MIDI Output

To verify MIDI is working:

1. **Install MIDI Monitor**: Download from https://www.snoize.com/MIDIMonitor/
2. **Run MIDI Monitor**
3. **Run GRUVBOK**
4. You should see MIDI note messages in MIDI Monitor

Or use a software synthesizer:

```bash
# Install FluidSynth
brew install fluidsynth

# Download a soundfont
wget https://download.musescore.com/soundfont/MuseScore_General/MuseScore_General.sf3

# Run FluidSynth (in another terminal)
fluidsynth -a coreaudio MuseScore_General.sf3

# Run GRUVBOK and you should hear sounds!
```

## Development Build

For development with better error messages:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(sysctl -n hw.ncpu)
```

For optimized release build:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
```

## Cleaning Up

To clean and rebuild from scratch:

```bash
cd build
rm -rf *
cmake ..
make -j$(sysctl -n hw.ncpu)
```

## Next Steps

Once you have it building and running:

1. **Read CLAUDE.md** - Understand the architecture
2. **Modify the test pattern** - Edit `src/desktop/main.cpp` to create different patterns
3. **Create new modes** - Copy `modes/TEMPLATE.lua` and implement your own
4. **Add interactive keyboard input** - Currently it just plays the test pattern

## Getting Help

If you encounter issues:

1. Check that all dependencies are installed: `brew list lua rtmidi cmake`
2. Make sure you're running from the project root directory
3. Look at the console output for error messages
4. Check `CLAUDE.md` for architecture details

Happy music making! 🎵
