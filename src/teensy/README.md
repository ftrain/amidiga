# GRUVBOK Teensy 4.1 Port

## Status: ✅ Ready for Hardware Testing

The Teensy 4.1 hardware implementation is **complete**. Firmware is ready for deployment to physical hardware.

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

### 1. Lua Library Integration ✅ READY
**Status:** Library structure complete, awaiting user to download Lua source

**Setup Required:**
```bash
cd lib/lua
curl -R -O http://www.lua.org/ftp/lua-5.4.6.tar.gz
tar zxf lua-5.4.6.tar.gz
cp lua-5.4.6/src/*.c .
cp lua-5.4.6/src/*.h .
rm -f lua.c luac.c onelua.c  # Remove standalone interpreters
```

**Optimization:** LUA_32BITS flag enabled (reduces memory ~30-40%)

### 2. SD Card Support ✅ IMPLEMENTED
**Status:** Complete - loads modes from SD card `/modes` directory

**Code in `main.cpp`:**
```cpp
if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("WARNING: SD card initialization failed!");
} else {
    int loaded = mode_loader->loadModesFromDirectory("/modes", 120);
    Serial.print("Successfully loaded ");
    Serial.print(loaded);
    Serial.println(" Lua modes");
}
```

**Recommended SD Card Structure:**
```
SD:/modes/
├── 00_song.lua
├── 01_drums.lua
├── 02_acid.lua
├── 03_chords.lua
└── 08-14 (experimental modes)
```

### 3. Song Save/Load ✅ IMPLEMENTED
**Status:** Complete - JSON format with Song::save() and Song::load()

**Format:** Sparse JSON encoding (see `docs/SONG_FORMAT.md`)
**Location:** Desktop saves to `/tmp/`, Teensy will use SD card `/songs/`

### 4. Physical Hardware Testing ⏳ PENDING
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

- ⚠️ Not tested on physical hardware (awaiting Teensy 4.1 device)
- ⚠️ Memory may exceed 1MB with all 15 Lua contexts (recommend starting with 8-12 modes)

## Next Steps

Priority order:
1. **Download Lua source** to `lib/lua/` (see Step 1 above)
2. **Build firmware:** `platformio run -e teensy41`
3. **Prepare SD card:** Format FAT32, copy modes to `/modes`
4. **Upload firmware:** `platformio run -e teensy41 --target upload`
5. **Test on hardware:** Buttons, pots, MIDI, LED
6. **Profile memory:** Confirm <800KB RAM usage

**See:** `docs/TEENSY_DEPLOYMENT_GUIDE.md` for complete walkthrough

## Questions?

- See `docs/LUA_API.md` for Lua mode development
- See `CLAUDE.md` for overall architecture
- See desktop implementation in `src/desktop/` for reference
