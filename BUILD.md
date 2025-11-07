# Building GRUVBOK

This guide explains how to build and run GRUVBOK on macOS and Linux.

## Prerequisites

### macOS

```bash
# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake lua sdl2
```

### Linux (Debian/Ubuntu)

```bash
# Install dependencies
sudo apt install cmake lua5.4 liblua5.4-dev libsdl2-dev libasound2-dev
```

**Note:** RtMidi and Dear ImGui are bundled in `external/` - no installation needed!

## Building

### Quick Build

```bash
# From project root
cmake -B build && cmake --build build

# Run the GUI
bin/gruvbok
```

### Step-by-Step

```bash
# 1. Create build directory
mkdir -p build
cd build

# 2. Configure with CMake
cmake ..

# 3. Build
make -j$(nproc)  # Linux
# or
make -j$(sysctl -n hw.ncpu)  # macOS

# 4. Run (from project root)
cd ..
bin/gruvbok
```

The executable will be in `bin/gruvbok`.

## Expected Output

GRUVBOK opens a GUI window with:
- **Main Window:** Pattern grid, step buttons, rotary/slider controls
- **Song Data Explorer:** Hierarchical view of all event data
- **System Log:** MIDI messages and system events

You should see MIDI clock messages immediately if connected to a DAW/synthesizer.

## Troubleshooting

### CMake can't find Lua

**macOS:**
```bash
brew info lua  # Check installation path
cmake -B build -DLUA_INCLUDE_DIR=/opt/homebrew/include/lua -DLUA_LIBRARIES=/opt/homebrew/lib/liblua.dylib
```

**Linux:**
```bash
sudo apt install liblua5.4-dev lua5.4
```

### CMake can't find SDL2

**macOS:**
```bash
brew install sdl2
```

**Linux:**
```bash
sudo apt install libsdl2-dev
```

### No MIDI Output

**macOS:** Enable IAC Driver
1. Open **Audio MIDI Setup** (`/Applications/Utilities/`)
2. **Window → Show MIDI Studio**
3. Double-click **IAC Driver**, check "**Device is online**"

**Linux:** ALSA MIDI should work automatically

**Test:** Install MIDI Monitor (macOS) or `aseqdump` (Linux) to verify MIDI output

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

## Build Types

**Debug (default):**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

**Release (optimized):**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Clean Rebuild

```bash
rm -rf build bin
cmake -B build && cmake --build build
```

## Running Tests

```bash
cd build
ctest --verbose
```

All 56 tests should pass ✅

## Next Steps

- **QUICKSTART.md** - Get up and running quickly
- **CLAUDE.md** - Understand the architecture
- **docs/LUA_API.md** - Create custom modes
- **DEVELOPMENT_ROADMAP.md** - See project status

**Create patterns:** Use the GUI to click step buttons and adjust sliders
**Save songs:** Click "Save Song" button (saves to `/tmp/gruvbok_song_*.json`)
**Load songs:** Click "Load Song" button

Happy music making! 🎵
