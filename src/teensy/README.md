# GRUVBOK Teensy 4.1 Port

## Status: 🚧 Work in Progress

The Teensy 4.1 hardware implementation is **partially complete**. Core infrastructure is ready, but some integration work remains.

## What's Implemented ✅

### Hardware Abstraction (`teensy_hardware.h/cpp`)
- ✅ TeensyHardware class implementing HardwareInterface
- ✅ Button input (B1-B16) with debouncing
- ✅ Rotary pots (R1-R4) with ADC reading and filtering
- ✅ Slider pots (S1-S4) with ADC reading and filtering
- ✅ LED tempo indicator control
- ✅ USB MIDI output (Note On/Off, CC, Clock, Start/Stop)
- ✅ Millisecond timing (getMillis())

### Pin Mappings
```
Buttons (INPUT_PULLUP, active-low):
  B1-B8:   Pins 0-7
  B9-B16:  Pins 8-15

Rotary Pots (Analog input, 10-bit ADC):
  R1 (Mode):    A0 (pin 14)
  R2 (Tempo):   A1 (pin 15)
  R3 (Pattern): A2 (pin 16)
  R4 (Track):   A3 (pin 17)

Slider Pots (Analog input, 10-bit ADC):
  S1: A4 (pin 18)
  S2: A5 (pin 19)
  S3: A6 (pin 20)
  S4: A7 (pin 21)

LED:
  Tempo LED: Pin 13 (onboard LED)
```

### Main Firmware (`main.cpp`)
- ✅ Setup routine initializing hardware, song, engine
- ✅ Main update loop calling engine->update()
- ✅ Serial debug output
- ✅ Status printing every 5 seconds

### Build Configuration (`platformio.ini`)
- ✅ Teensy 4.1 board configuration
- ✅ C++17 standard
- ✅ USB MIDI enabled
- ✅ Upload settings

## What's Pending ⏳

### 1. Lua Library Integration
**Status:** Not yet implemented
**Required:** Lua 5.4 source needs to be compiled for Teensy

**To Do:**
```bash
# 1. Download Lua 5.4.6 source
curl -R -O http://www.lua.org/ftp/lua-5.4.6.tar.gz
tar zxf lua-5.4.6.tar.gz

# 2. Copy to lib/ directory
mkdir -p lib/lua
cp -r lua-5.4.6/src/* lib/lua/

# 3. Create lib/lua/library.json for PlatformIO
# 4. Modify lua.h to disable features not needed (reduce memory)
```

**Challenges:**
- Lua needs ~50-100KB RAM per context (15 contexts = ~750KB-1.5MB)
- May need to reduce to fewer simultaneous modes or optimize Lua
- Consider compiling Lua with LUA_32BITS to reduce memory

### 2. SD Card Support
**Status:** Not yet implemented
**Required:** Load .lua mode files from SD card

**To Do:**
```cpp
#include <SD.h>

void setup() {
    // ... existing code ...

    // Initialize SD card
    if (!SD.begin(BUILTIN_SDCARD)) {
        Serial.println("ERROR: SD card not found!");
        while (1);
    }

    // Load modes from SD
    mode_loader = new ModeLoader("/modes");  // Path on SD card
}
```

**Mode Files on SD:**
```
SD:/
├── modes/
│   ├── 00_song.lua
│   ├── 01_drums.lua
│   ├── 02_acid.lua
│   ├── 03_chords.lua
│   ├── 04_arpeggiator.lua
│   ├── 05_euclidean.lua
│   ├── 06_random.lua
│   └── 07_samplehold.lua
└── songs/
    ├── demo.grv (optional: song save format)
    └── mysong.grv
```

### 3. Song Save/Load
**Status:** Not yet implemented
**Required:** Persist songs to SD card

**Proposed Format:** JSON or binary format
(See "Design song save format" task)

### 4. Physical Hardware Testing
**Status:** Not tested on real hardware
**Required:** Actual Teensy 4.1 with buttons, pots, MIDI

**Test Checklist:**
- [ ] All 16 buttons respond correctly
- [ ] Button debouncing works (no double-triggers)
- [ ] Rotary pots (R1-R4) read correctly
- [ ] Slider pots (S1-S4) read correctly
- [ ] LED blinks on beat
- [ ] USB MIDI output works (connect to DAW)
- [ ] MIDI clock synchronization works
- [ ] Timing is accurate (no jitter)
- [ ] Memory usage is within limits (<1MB)

### 5. Memory Optimization
**Status:** Not yet profiled
**Required:** Ensure firmware fits in 1MB RAM

**Estimated Memory Usage:**
```
Event data:  ~245 KB (15 modes × 32 patterns × 8 tracks × 16 events × 4 bytes)
Lua states:  ~750 KB (15 contexts × 50 KB each) - MAY BE TOO MUCH!
Code/stack:  ~300 KB
Total:       ~1295 KB - EXCEEDS 1MB!
```

**Optimization Strategies:**
1. Reduce Lua to 8 active modes instead of 15
2. Use compiled Lua bytecode instead of source (smaller)
3. Disable unused Lua libraries
4. Use LUA_32BITS build option
5. Consider JIT-compiling only active mode
6. Share Lua state between some modes

## Building for Teensy

### Prerequisites
```bash
# Install PlatformIO
pip install platformio

# Or use PlatformIO IDE (VS Code extension)
```

### Build
```bash
# From project root
platformio run

# Upload to Teensy
platformio run --target upload

# Monitor serial output
platformio device monitor
```

### Expected Output
```
========================================
GRUVBOK - Teensy 4.1 Firmware
========================================
Initializing hardware...
Hardware initialized successfully
Creating song...
Song created
Loading Lua modes...
WARNING: Lua mode loading not yet implemented for Teensy
         Using placeholder modes (will output no MIDI)
Creating engine...
Engine created
Starting playback...
Playback started
========================================
GRUVBOK is ready!
Tempo: 120 BPM
Mode: 1 (Drums)
Pattern: 0
Track: 0
========================================
```

## Development Workflow

1. **Test on Desktop First**: Always test changes in desktop simulator before Teensy
2. **Compile for Teensy**: Use PlatformIO to check for compilation errors
3. **Upload to Hardware**: Flash to Teensy and test with actual buttons/pots
4. **Profile Memory**: Use `arm-none-eabi-size` to check RAM/Flash usage
5. **Iterate**: Fix issues, optimize, repeat

## Known Issues

- ⚠️ Lua not yet integrated (modes won't work)
- ⚠️ SD card not yet supported (can't load mode files)
- ⚠️ Memory may exceed 1MB with 15 Lua contexts
- ⚠️ Not tested on real hardware
- ⚠️ No song save/load functionality

## Next Steps

See the pending tasks above. Priority order:
1. Integrate Lua library
2. Add SD card support
3. Test on real hardware
4. Profile and optimize memory
5. Add song save/load

## Questions?

- See `docs/LUA_API.md` for Lua mode development
- See `CLAUDE.md` for overall architecture
- See desktop implementation in `src/desktop/` for reference
