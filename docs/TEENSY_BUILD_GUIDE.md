# GRUVBOK Teensy 4.1 Build Guide

## Quick Status

**Current State:**
✅ TeensyHardware class fully implemented
✅ Main firmware entry point (setup/loop)
✅ PlatformIO configuration ready
⏳ Lua integration pending
⏳ SD card support pending

**What Works Right Now:**
- Button reading (B1-B16) with debouncing
- Pot reading (R1-R4, S1-S4) with filtering
- LED tempo indicator with PWM brightness
- USB MIDI output (notes, CC, clock)
- Core engine playback loop

**What's Missing:**
- Lua mode loading (modes won't generate MIDI yet)
- SD card file I/O
- Physical hardware testing

---

## Prerequisites

### Hardware

**Required:**
- Teensy 4.1 board (600 MHz ARM Cortex-M7, 1MB RAM)
- USB cable (micro-USB for Teensy 4.1)

**For Full GRUVBOK:**
- 16x momentary buttons (B1-B16)
- 4x rotary potentiometers (R1-R4) - 10kΩ linear
- 4x slide potentiometers (S1-S4) - 10kΩ linear
- Breadboard or custom PCB
- Jumper wires
- Optional: External LED for tempo indicator

**Wiring (see `src/teensy/README.md` for pin mappings):**
```
Buttons: Pins 0-15 (INPUT_PULLUP, active-low)
Rotary Pots: A0-A3 (analog input)
Slider Pots: A4-A7 (analog input)
LED: Pin 13 (built-in LED, or external with resistor)
```

### Software

1. **PlatformIO** (recommended)
   ```bash
   # Via pip
   pip install platformio

   # Or use VS Code extension
   # https://platformio.org/install/ide?install=vscode
   ```

2. **Teensy Loader** (included with PlatformIO Teensy platform)
   - Auto-installed when you first build

3. **USB MIDI Driver** (for your computer to receive MIDI)
   - macOS: Built-in
   - Linux: ALSA MIDI
   - Windows: Teensy USB MIDI driver (auto-detected)

---

## Building the Firmware

### Step 1: Navigate to Project Root

```bash
cd /path/to/amidiga
```

### Step 2: Build for Teensy

```bash
platformio run -e teensy41
```

**Expected Output:**
```
Processing teensy41 (platform: teensy; board: teensy41; framework: arduino)
...
Building in release mode
...
Linking .pio/build/teensy41/firmware.elf
Building .pio/build/teensy41/firmware.hex
...
SUCCESS
```

**Build Artifacts:**
- `.pio/build/teensy41/firmware.hex` - Main firmware file
- `.pio/build/teensy41/firmware.elf` - Executable with debug symbols

### Step 3: Upload to Teensy

**Method 1: Via PlatformIO (automatic)**
```bash
platformio run -e teensy41 --target upload
```

- Teensy Loader GUI will open automatically
- Press the button on your Teensy board
- Firmware uploads automatically

**Method 2: Manual (Teensy Loader GUI)**
```bash
# 1. Build firmware
platformio run -e teensy41

# 2. Open Teensy Loader application
# 3. File → Open HEX File → select .pio/build/teensy41/firmware.hex
# 4. Press button on Teensy
# 5. Click "Program" in Teensy Loader
```

### Step 4: Monitor Serial Output

```bash
platformio device monitor -e teensy41
```

**Expected Output:**
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
         To enable: Add SD card support and call mode_loader->loadModesFromDirectory("/modes", 120)
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
Status: Mode=1 Pattern=0 Track=0 Step=0 Tempo=120 BPM
```

---

## Checking Build Size

Verify your firmware fits in Teensy memory:

```bash
# After building
pio run -e teensy41 -t size
```

**Output Example:**
```
Memory Usage (Teensy 4.1):
DATA:    [====      ]  35.2% (used 368KB out of 1024KB)
PROGRAM: [===       ]  28.5% (used 2MB out of 7.9MB)
```

**Safe Limits:**
- DATA (RAM): Should stay under 800KB (leave margin for stack)
- PROGRAM (Flash): Plenty of space (7.9MB available)

---

## Testing Without Lua Modes

Even without Lua integration, you can test hardware:

1. **Upload firmware**
2. **Connect MIDI monitor** (e.g., MIDI Monitor on macOS, MIDI-OX on Windows)
3. **Press buttons B1-B16** → Engine will step through patterns
4. **Twist pots R1-R4** → Change mode, tempo, pattern, track
5. **Watch LED on pin 13** → Blinks on beat (tempo indicator)

**What you'll see:**
- LED blinks every quarter note
- MIDI Clock messages at 24 PPQN
- MIDI Start/Stop messages
- **No Note On/Off yet** (requires Lua modes)

---

## Adding Lua Support (TODO)

### Option 1: Compile Lua from Source

**Download Lua 5.4.6:**
```bash
cd lib/
curl -R -O http://www.lua.org/ftp/lua-5.4.6.tar.gz
tar zxf lua-5.4.6.tar.gz
mv lua-5.4.6/src lua
rm -rf lua-5.4.6 lua-5.4.6.tar.gz
```

**Create `lib/lua/library.json`:**
```json
{
  "name": "lua",
  "version": "5.4.6",
  "build": {
    "flags": [
      "-DLUA_32BITS",
      "-DLUA_USE_POSIX"
    ],
    "srcFilter": [
      "+<*.c>",
      "-<lua.c>",
      "-<luac.c>"
    ]
  }
}
```

**Update `platformio.ini`:**
```ini
lib_deps =
    lua
```

**Memory Concern:**
Lua may require 50-100KB per context. With 15 modes = 750KB-1.5MB, which exceeds Teensy's 1MB RAM.

**Solutions:**
- Reduce to 8 active modes (400-800KB)
- Use compiled Lua bytecode
- Share Lua state between some modes
- Optimize with `LUA_32BITS` and minimal libraries

### Option 2: Pre-compiled Lua Bytecode (Smaller)

Compile `.lua` files to bytecode on desktop:
```bash
luac -o 01_drums.luac modes/01_drums.lua
```

Load bytecode instead of source (saves RAM).

---

## Adding SD Card Support (TODO)

### Hardware

Teensy 4.1 has a **built-in microSD card slot** on the underside.

### Software

**Update `src/teensy/main.cpp`:**
```cpp
#include <SD.h>

void setup() {
    // ... existing setup ...

    // Initialize SD card
    Serial.println("Initializing SD card...");
    if (!SD.begin(BUILTIN_SDCARD)) {
        Serial.println("ERROR: SD card initialization failed!");
        while (1) {
            delay(1000);  // Halt
        }
    }
    Serial.println("SD card initialized");

    // Load Lua modes from SD
    int loaded = mode_loader->loadModesFromDirectory("/modes", 120);
    Serial.print("Loaded ");
    Serial.print(loaded);
    Serial.println(" modes from SD card");

    // ... rest of setup ...
}
```

**Prepare SD Card:**
```
Format: FAT32
Structure:
  /modes/
    00_song.lua
    01_drums.lua
    02_acid.lua
    03_chords.lua
    ...
  /songs/
    (optional: saved songs in JSON format)
```

**Copy Lua modes to SD:**
```bash
cp modes/*.lua /path/to/sd_card/modes/
```

---

## Troubleshooting

### Build Errors

**"fatal error: Arduino.h: No such file or directory"**
- Solution: Install Teensy platform
  ```bash
  platformio platform install teensy
  ```

**"undefined reference to 'usbMIDI'"**
- Solution: Ensure `-DUSB_MIDI` is in `platformio.ini` build_flags
- Check that USB Type is set to "Serial + MIDI" (auto-configured by PlatformIO)

**"undefined reference to lua_*"**
- Solution: Lua not yet integrated (see "Adding Lua Support" above)

### Upload Errors

**"Teensy not found"**
- Press the physical button on Teensy board
- Try different USB cable
- Check USB connection

**"Permission denied" (Linux)**
```bash
sudo usermod -a -G dialout $USER
# Log out and back in
```

### Runtime Issues

**No MIDI output**
- Check USB MIDI connection (Device Manager on Windows, Audio MIDI Setup on macOS)
- Verify firmware uploaded successfully (should see serial output)
- **Expected:** MIDI Clock and Start/Stop work, but no notes (Lua not loaded)

**LED doesn't blink**
- Check pin 13 is LED_PIN in `teensy_hardware.h`
- Verify tempo is reasonable (60-240 BPM)
- LED should blink every quarter note (4 steps)

**Buttons don't respond**
- Check wiring (INPUT_PULLUP = button connects pin to GND)
- Verify button pins in `teensy_hardware.h` (default 0-15)
- Monitor serial output for debugging

---

## Next Steps

### For Developers

1. **Add Lua integration** (see above)
2. **Add SD card support** (see above)
3. **Test on real hardware** (buttons, pots, MIDI output)
4. **Profile memory usage** with all modes loaded
5. **Optimize** if RAM usage exceeds safe limits

### For Users

1. **Wait for Lua integration** to be completed
2. **Or contribute!** Help integrate Lua library
3. **Test hardware** even without Lua (button/pot/LED testing)

---

## Reference

- **Teensy 4.1 Info:** https://www.pjrc.com/store/teensy41.html
- **PlatformIO Teensy:** https://docs.platformio.org/en/latest/platforms/teensy.html
- **Lua 5.4 Manual:** https://www.lua.org/manual/5.4/
- **Project Architecture:** See `CLAUDE.md`
- **Lua API:** See `docs/LUA_API.md`
- **Pin Mappings:** See `src/teensy/README.md`

---

## Questions?

Open an issue or check `CLAUDE.md` for architecture details.
