# GRUVBOK Teensy 4.1 Guide

Complete guide for building and deploying GRUVBOK firmware to Teensy 4.1 hardware.

---

## Table of Contents

1. [Current Status](#current-status)
2. [Prerequisites](#prerequisites)
3. [Quick Start](#quick-start)
4. [Pin Mappings](#pin-mappings)
5. [Building Firmware](#building-firmware)
6. [Uploading to Teensy](#uploading-to-teensy)
7. [SD Card Setup](#sd-card-setup)
8. [Testing](#testing)
9. [Troubleshooting](#troubleshooting)
10. [Memory Optimization](#memory-optimization)

---

## Current Status

**Implementation Status:** ✅ Ready for Hardware Testing

### What's Implemented ✅

**Core Firmware:**
- ✅ TeensyHardware class implementing HardwareInterface
- ✅ Button input (B1-B16) with debouncing
- ✅ Rotary pots (R1-R4) with ADC reading and filtering
- ✅ Slider pots (S1-S4) with ADC reading and filtering
- ✅ LED tempo indicator control
- ✅ USB MIDI output (Note On/Off, CC, Clock, Start/Stop)
- ✅ Main firmware setup/loop structure
- ✅ Lua integration (loads modes from SD card)
- ✅ SD card support with SdFat library
- ✅ Song save/load (JSON format)

**Build System:**
- ✅ PlatformIO configuration for Teensy 4.1
- ✅ C++17 standard
- ✅ USB MIDI enabled
- ✅ Lua library integration

### What's Pending ⏳

- ⏳ Physical hardware testing (awaiting Teensy 4.1 device)
- ⏳ Memory profiling with all modes loaded
- ⏳ Performance optimization if needed

---

## Prerequisites

### Hardware Required

**Essential:**
- **Teensy 4.1** (~$30)
  - 600 MHz ARM Cortex-M7
  - 1MB RAM, 8MB Flash
  - Built-in microSD card slot
  - USB MIDI support
- **MicroSD Card** (1GB+, formatted FAT32)
- **USB Cable** (micro-USB)

**For Full Groovebox:**
- 16× momentary buttons (B1-B16)
- 4× 10kΩ rotary pots (R1-R4)
- 4× 10kΩ slide pots (S1-S4)
- Breadboard or custom PCB
- Jumper wires
- Optional: External LED for tempo indicator

### Software Required

1. **PlatformIO**
   ```bash
   pip install platformio
   # Or use VS Code extension
   ```

2. **Python 3** (for automated Lua download script)
   - Usually pre-installed on macOS/Linux
   - Windows: https://www.python.org/downloads/

3. **Git** (for cloning repository)

**Note:** Lua 5.4.6 source downloads automatically on first build via `scripts/embed_lua.py`

---

## Quick Start

### Step 1: Clone Repository

```bash
git clone <repository-url>
cd amidiga
```

### Step 2: Prepare SD Card

**Format:** FAT32

**Directory Structure:**
```
SD:/
├── modes/
│   ├── 00_song.lua
│   ├── 01_drums.lua
│   ├── 02_acid.lua
│   ├── 03_chords.lua
│   └── (other modes)
└── songs/ (optional, for future)
```

**Copy Lua Modes:**
```bash
# From project root
mkdir /path/to/SD/modes
cp modes/*.lua /path/to/SD/modes/

# Mac: cp modes/*.lua /Volumes/SDCARD/modes/
# Windows: Use File Explorer
```

**Recommended for First Test:**
Start with 4 modes to save memory:
- `00_song.lua` - Pattern sequencer
- `01_drums.lua` - Drum machine
- `02_acid.lua` - Bassline
- `03_chords.lua` - Chord player

### Step 3: Build Firmware

**First build automatically downloads Lua 5.4.6:**

```bash
platformio run -e teensy41
```

The build script will:
1. Download Lua 5.4.6 from lua.org (if not already present)
2. Extract and copy source files to `lib/lua/`
3. Configure for Teensy (32-bit mode, C89 compatibility)
4. Build firmware

**Check Memory Usage:**
```bash
pio run -e teensy41 -t size
```

**Expected:**
```
DATA:    [====      ]  40-60% (used 400-600KB out of 1MB)
PROGRAM: [===       ]  30-40% (used 2-3MB out of 7.9MB)
```

**Memory Guidelines:**
- DATA < 800KB: ✅ Safe
- DATA 800-950KB: ⚠️ Tight (reduce modes if needed)
- DATA > 950KB: ❌ Too much (will crash)

### Step 4: Upload to Teensy

1. Connect Teensy to computer via USB
2. Run:
   ```bash
   platformio run -e teensy41 --target upload
   ```
3. Press the **white button** on Teensy board
4. Firmware uploads automatically

### Step 5: Insert SD Card and Monitor

1. **Power off Teensy** (unplug USB)
2. Insert microSD card into Teensy's slot (underside of board)
3. Reconnect USB
4. Monitor serial output:
   ```bash
   platformio device monitor -e teensy41
   ```

**Expected Output (Success):**
```
========================================
GRUVBOK - Teensy 4.1 Firmware
========================================
Initializing hardware...
Hardware initialized successfully
Creating song...
Song created
Initializing SD card...
SD card initialized successfully
Loading Lua modes from SD:/modes/...
Successfully loaded 4 Lua modes
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

---

## Pin Mappings

### Buttons (INPUT_PULLUP, active-low)

Connect buttons between pin and GND:

```
B1-B8:   Pins 0-7
B9-B16:  Pins 8-15
```

When button pressed: Pin reads LOW (connects to GND)
When released: Pin reads HIGH (pulled up internally)

### Potentiometers (Analog input, 10-bit ADC)

**Rotary Pots (Global controls):**
```
R1 (Mode):    A0 (pin 14)
R2 (Tempo):   A1 (pin 15)
R3 (Pattern): A2 (pin 16)
R4 (Track):   A3 (pin 17)
```

**Slider Pots (Mode parameters):**
```
S1: A4 (pin 18)
S2: A5 (pin 19)
S3: A6 (pin 20)
S4: A7 (pin 21)
```

**Wiring:** Center pin to Teensy analog pin, outer pins to 3.3V and GND

### LED

```
Tempo LED: Pin 13 (onboard LED, or external with 220Ω resistor)
```

Blinks every quarter note (4 steps)

---

## Building Firmware

### First Build

```bash
# Navigate to project root
cd /path/to/amidiga

# Build
platformio run -e teensy41
```

**Build Artifacts:**
- `.pio/build/teensy41/firmware.hex` - Main firmware file
- `.pio/build/teensy41/firmware.elf` - Executable with debug symbols

### Check Build Size

```bash
pio run -e teensy41 -t size
```

**Example Output:**
```
Memory Usage (Teensy 4.1):
DATA:    [====      ]  35.2% (used 368KB out of 1024KB)
PROGRAM: [===       ]  28.5% (used 2MB out of 7.9MB)
```

**Safe Limits:**
- DATA (RAM): < 800KB (leave margin for stack)
- PROGRAM (Flash): < 7.9MB (plenty of space)

### Clean Build

```bash
# Clean and rebuild
platformio run -e teensy41 -t clean
platformio run -e teensy41
```

---

## Uploading to Teensy

### Method 1: Automatic (PlatformIO)

```bash
platformio run -e teensy41 --target upload
```

1. Teensy Loader GUI opens automatically
2. Press button on Teensy board
3. Firmware uploads
4. LED blinks when done

### Method 2: Manual (Teensy Loader)

```bash
# Build firmware first
platformio run -e teensy41

# Then use Teensy Loader GUI:
# 1. File → Open HEX File → .pio/build/teensy41/firmware.hex
# 2. Press button on Teensy
# 3. Click "Program"
```

---

## SD Card Setup

### Formatting

Format microSD card as **FAT32**

### Directory Structure

```
SD:/
├── modes/          # Lua mode scripts (required)
│   ├── 00_song.lua
│   ├── 01_drums.lua
│   ├── 02_acid.lua
│   └── ...
└── songs/          # Saved songs (optional, for future)
```

### Copying Modes

**From Desktop:**
```bash
cp modes/*.lua /path/to/SD/modes/
```

**Recommended Modes for Testing:**
- Start with 4-6 modes
- Add more after confirming it works
- Each mode uses ~50-100KB RAM

### Lua Mode Loading

Modes are loaded on startup from `/modes` directory:
- File naming: `NN_name.lua` (e.g., `01_drums.lua`)
- Mode number from filename prefix
- Loading timeout: 120 seconds total
- Each mode gets its own Lua context

---

## Testing

### Test Without Hardware

Even without buttons/pots, you can test:

1. **Upload firmware**
2. **Connect MIDI monitor** (MIDI Monitor on macOS, MIDI-OX on Windows)
3. **Watch for:**
   - LED blinks every quarter note
   - MIDI Clock messages at 24 PPQN
   - MIDI Start/Stop messages

### Test With Hardware

**Buttons:**
- Press any button → Serial shows event changes
- Step through patterns in real-time

**Pots:**
- Turn rotary pots → Mode/Tempo/Pattern/Track changes
- Move sliders → Parameter values change

**LED:**
- Should blink on beat (every 4 steps)
- Tempo range: 60-240 BPM

### Test MIDI Output

**Connect to DAW:**
1. **Windows:** Teensy shows as USB MIDI device (auto-detected)
2. **Mac/Linux:** Appears as "Teensy MIDI"
3. Create MIDI track in DAW
4. Select input: "Teensy MIDI"
5. Load a synth
6. Program patterns with buttons
7. Hear sound!

### Test Checklist

- [ ] All 16 buttons respond correctly
- [ ] Button debouncing works (no double-triggers)
- [ ] Rotary pots (R1-R4) read correctly
- [ ] Slider pots (S1-S4) read correctly
- [ ] LED blinks on beat
- [ ] USB MIDI output works
- [ ] MIDI clock synchronization works
- [ ] Timing is accurate (no jitter)
- [ ] Memory usage is within limits

---

## Troubleshooting

### Build Errors

**"lua.h: No such file or directory"**
- **Solution:** The `embed_lua.py` script should download Lua automatically on first build
- Check that `scripts/embed_lua.py` exists
- Try cleaning and rebuilding: `pio run -e teensy41 -t clean && pio run -e teensy41`
- If still failing, manually run: `python scripts/embed_lua.py`

**"undefined reference to lua_*"**
- **Solution:** Lua source files missing or incomplete
- Run the embed script manually: `python scripts/embed_lua.py`

**Memory exceeds 1MB:**
```
DATA: [==========] 110% (used 1.1MB out of 1MB)
```
- **Solution:** Reduce number of modes on SD card to 6-8
- Or: Enable optimizations in `lib/lua/library.json`

**"fatal error: Arduino.h: No such file or directory"**
- **Solution:** Install Teensy platform:
  ```bash
  platformio platform install teensy
  ```

### Upload Errors

**"Teensy not found"**
- Press physical button on Teensy
- Try different USB port/cable
- Reinstall Teensy drivers

**"Permission denied" (Linux):**
```bash
sudo usermod -a -G dialout $USER
# Log out and back in
```

### Runtime Errors

**No MIDI output:**
- Check: Playback started? (Serial says "Playback started")
- Check: Modes loaded? (Serial says "Successfully loaded N modes")
- Check: Events programmed? (Use desktop to create patterns first)

**LED doesn't blink:**
- LED should blink every quarter note (4 steps)
- Verify tempo is reasonable (60-240 BPM)
- Check: Engine playing? (Serial says "Playback started")

**SD card initialization failed:**
```
WARNING: SD card initialization failed!
```
- Check: SD card inserted?
- Check: Formatted FAT32?
- Try reinserting card
- Try different SD card

**No Lua modes found:**
```
WARNING: No Lua modes found on SD card
```
- Check: `/modes/` directory exists on SD?
- Check: `.lua` files present?
- Check: Files named correctly (e.g., `01_drums.lua`)?

**Buttons don't respond:**
- Wiring: Buttons connect pin to GND when pressed (INPUT_PULLUP)
- Check pin numbers match `BUTTON_PINS` in `teensy_hardware.h`
- Monitor serial output for debugging

**Pots jitter or read incorrectly:**
- Check wiring: center pin to analog input, outers to 3.3V/GND
- Adjust filtering in `teensy_hardware.cpp` if needed
- Verify pots are 10kΩ linear

---

## Memory Optimization

If you hit memory limits, try these strategies:

### 1. Reduce Number of Modes

Only load 6-8 modes instead of all 15:
```
SD:/modes/
  00_song.lua    (essential - pattern sequencer)
  01_drums.lua
  02_acid.lua
  03_chords.lua
  04_arpeggiator.lua
  05_euclidean.lua
```

**Estimated Memory:**
- 6 modes × 50KB = ~300KB
- Leaves plenty of headroom

### 2. Use Compiled Lua Bytecode (Advanced)

Compile scripts to bytecode on desktop:
```bash
luac -o 01_drums.luac modes/01_drums.lua
```

Modify ModeLoader to load `.luac` files.
Bytecode is smaller and loads faster.

### 3. Disable Unused Lua Libraries

Edit `lib/lua/linit.c` and comment out libraries you don't need:
```c
// {"io", luaopen_io},     // File I/O not needed
// {"os", luaopen_os},     // OS functions not needed
```

### 4. Optimize Lua Build Flags

In `lib/lua/library.json`:
```json
{
  "build": {
    "flags": [
      "-DLUA_32BITS",           // Use 32-bit integers (smaller)
      "-DLUA_USE_POSIX"
    ]
  }
}
```

### 5. Profile Memory Usage

After building:
```bash
arm-none-eabi-size --format=SysV .pio/build/teensy41/firmware.elf
```

Look for largest sections and optimize accordingly.

### Estimated Memory Breakdown

```
Event data:    ~245 KB  (15 modes × 32 patterns × 8 tracks × 16 events × 4 bytes)
Lua contexts:  ~300-750 KB  (6-15 modes × 50 KB each)
Code/stack:    ~200-300 KB
---------------------------------------------
Total:         ~745-1295 KB

Target:        < 800 KB (safe for Teensy 4.1)
```

**Recommendation:** Start with 6-8 modes, add more as memory allows.

---

## Development Workflow

**Best Practice: Desktop First, Then Teensy**

1. **Test on Desktop First**
   - Edit code in `src/` or Lua modes in `modes/`
   - Build desktop version: `cmake --build build`
   - Run GUI: `bin/gruvbok`
   - Verify behavior and create patterns

2. **Compile for Teensy**
   - Build: `platformio run -e teensy41`
   - Check memory: `pio run -e teensy41 -t size`
   - Ensure DATA < 800KB

3. **Upload and Test**
   - Flash to Teensy: `pio run -e teensy41 --target upload`
   - Monitor serial: `pio device monitor -e teensy41`
   - Test with actual buttons/pots

4. **Iterate**
   - Fix issues on desktop first
   - Then rebuild for Teensy
   - Repeat until working

---

## Creating Custom Modes

### Using Desktop First

1. Copy template:
   ```bash
   cp modes/TEMPLATE.lua modes/08_mymode.lua
   ```

2. Edit and test on desktop:
   ```bash
   bin/gruvbok  # GUI version
   ```

3. Copy to SD card:
   ```bash
   cp modes/08_mymode.lua /path/to/SD/modes/
   ```

4. Power cycle Teensy to reload modes

### Lua API Reference

See `docs/LUA_API.md` for complete API documentation.

**Quick Example:**
```lua
function init(context)
    -- Called once on mode load
end

function process_event(track, event)
    if event.switch then
        local pitch = event.pots[1]
        local velocity = event.pots[2]

        note(pitch, velocity)   -- MIDI note on
        off(pitch, 100)         -- Note off after 100ms
    end
end
```

---

## Song Save/Load (Future)

Song save/load to SD card is implemented in core but not yet exposed to hardware UI.

**To Enable:**
1. Add "Save" and "Load" button combinations (e.g., hold B1+B16)
2. Write JSON files to `SD:/songs/`
3. Add UI for file selection

**Current Implementation:**
- `Song::save(filename)` - Saves to JSON (sparse encoding)
- `Song::load(filename)` - Loads from JSON
- Works on desktop, ready for Teensy with SD card

---

## Hardware Enclosure Ideas

GRUVBOK is designed as a desktop/tabletop groovebox. Consider:

- **3D-printed case** for Teensy + buttons/pots
- **Laser-cut acrylic** faceplate with labels
- **MIDI DIN output** (in addition to USB MIDI)
- **External power supply** (Teensy can run on 5V USB or 3.3-5.5V on Vin)
- **Knob caps** for pots (improve tactile feel)
- **LED mounting** for better visibility

---

## Reference & Resources

**Project Documentation:**
- `CLAUDE.md` - Complete technical architecture
- `docs/LUA_API.md` - Lua mode API reference
- `docs/SONG_FORMAT.md` - JSON song file format
- Main `README.md` - Project overview

**External Resources:**
- **Teensy 4.1:** https://www.pjrc.com/store/teensy41.html
- **PlatformIO Teensy:** https://docs.platformio.org/en/latest/platforms/teensy.html
- **Lua 5.4 Manual:** https://www.lua.org/manual/5.4/
- **MIDI Specification:** https://www.midi.org/specifications

---

## Success Checklist

Ready to deploy? Verify all steps:

- [ ] Repository cloned ✅
- [ ] SD card formatted FAT32 ✅
- [ ] Modes copied to `SD:/modes/` ✅
- [ ] Firmware builds successfully (Lua auto-downloads on first build) ✅
- [ ] Memory usage < 800KB ✅
- [ ] Firmware uploaded to Teensy ✅
- [ ] SD card inserted into Teensy ✅
- [ ] Serial monitor shows "Successfully loaded N modes" ✅
- [ ] LED blinks on beat ✅
- [ ] MIDI Clock visible in DAW ✅
- [ ] Buttons/pots respond (if wired) ✅

**You now have a working hardware GRUVBOK groovebox! 🎹🎛️🎉**

---

## Getting Help

- **Issues:** GitHub Issues
- **Architecture questions:** See `CLAUDE.md`
- **Lua API:** See `docs/LUA_API.md`
- **Pin mappings:** See this guide (Pin Mappings section)
