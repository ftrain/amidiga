# GRUVBOK Teensy 4.1 - Complete Deployment Guide

## Overview

This guide walks you through deploying GRUVBOK firmware to Teensy 4.1 hardware, step by step.

**What You'll Get:**
- Working hardware MIDI groovebox
- 8-15 simultaneous modes (depending on memory)
- USB MIDI output to your DAW
- SD card mode loading
- Real-time button/pot control

---

## Prerequisites

### Hardware Required

1. **Teensy 4.1 Board** (~$30)
   - 600 MHz ARM Cortex-M7
   - 1MB RAM, 8MB Flash
   - Built-in microSD card slot
   - USB MIDI support

2. **MicroSD Card** (formatted FAT32)
   - 1GB or larger
   - For storing Lua mode scripts

3. **USB Cable** (micro-USB)
   - For programming and power

4. **Optional (for full groovebox):**
   - 16× momentary buttons (B1-B16)
   - 4× 10kΩ rotary pots (R1-R4)
   - 4× 10kΩ slide pots (S1-S4)
   - Breadboard or custom PCB
   - Jumper wires

### Software Required

1. **PlatformIO** (recommended)
   ```bash
   pip install platformio
   # Or use VS Code extension
   ```

2. **Lua 5.4.6 Source** (you'll download this)

3. **Git** (for cloning repository)

---

## Step 1: Clone Repository

```bash
git clone <repository-url>
cd amidiga
```

---

## Step 2: Download and Install Lua Source

### Quick Method (Unix/Mac/Linux)

```bash
cd lib/lua

# Download Lua 5.4.6
curl -R -O http://www.lua.org/ftp/lua-5.4.6.tar.gz

# Extract
tar zxf lua-5.4.6.tar.gz

# Copy source files
cp lua-5.4.6/src/*.c .
cp lua-5.4.6/src/*.h .

# Remove standalone interpreters (not needed)
rm -f lua.c luac.c onelua.c

# Clean up
rm -rf lua-5.4.6 lua-5.4.6.tar.gz

cd ../..
```

### Manual Method (Windows)

1. Download: http://www.lua.org/ftp/lua-5.4.6.tar.gz
2. Extract with 7-Zip or WinRAR
3. Copy all `.c` and `.h` files from `lua-5.4.6/src/` to `lib/lua/`
4. Delete `lua.c`, `luac.c`, `onelua.c`

### Verification

```bash
ls lib/lua/*.c | wc -l  # Should show ~60 files
ls lib/lua/*.h | wc -l  # Should show ~20 files
```

You should see files like: `lapi.c`, `ltable.c`, `lua.h`, etc.

---

## Step 3: Prepare SD Card

### Format SD Card

1. Format as **FAT32**
2. Insert into your computer (use SD card reader if needed)

### Create Directory Structure

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
└── songs/ (optional, for future)
```

### Copy Lua Modes

```bash
# From project root
mkdir /media/SD_CARD/modes  # Adjust path for your system
cp modes/*.lua /media/SD_CARD/modes/

# Windows: Use File Explorer to copy modes/*.lua to SD:/modes/
# Mac: cp modes/*.lua /Volumes/SDCARD/modes/
```

### Recommended Modes for First Test

Start with these 4 modes to test (saves memory):
- `00_song.lua` - Pattern sequencer
- `01_drums.lua` - Drum machine
- `02_acid.lua` - Bassline
- `03_chords.lua` - Chord player

Add more later after confirming it works!

---

## Step 4: Build Firmware

### First Build (Test Compilation)

```bash
platformio run -e teensy41
```

**Expected Output:**
```
...
Building in release mode
Compiling .pio/build/teensy41/src/...
Linking .pio/build/teensy41/firmware.elf
Building .pio/build/teensy41/firmware.hex
SUCCESS
```

### Check Memory Usage

```bash
pio run -e teensy41 -t size
```

**Expected Output:**
```
DATA:    [====      ]  40-60% (used 400-600KB out of 1MB)
PROGRAM: [===       ]  30-40% (used 2-3MB out of 7.9MB)
```

**Memory Guidelines:**
- DATA < 800KB: ✅ Safe
- DATA 800-950KB: ⚠️ Tight (reduce modes if needed)
- DATA > 950KB: ❌ Too much (will crash)

If memory too high, reduce number of modes on SD card.

---

## Step 5: Upload to Teensy

### Connect Teensy

1. Connect Teensy to computer via USB
2. Teensy should appear as USB device

### Upload Firmware

```bash
platformio run -e teensy41 --target upload
```

**What Happens:**
1. Teensy Loader GUI opens automatically
2. Press the **white button** on Teensy board
3. Firmware uploads automatically
4. LED on Teensy blinks when done

### Alternative: Manual Upload

1. Build firmware: `pio run -e teensy41`
2. Open Teensy Loader app
3. File → Open HEX File → `.pio/build/teensy41/firmware.hex`
4. Press button on Teensy
5. Click "Program"

---

## Step 6: Insert SD Card and Test

### Insert SD Card

1. **Power off Teensy** (unplug USB)
2. Insert microSD card into Teensy's built-in slot (underside of board)
3. Reconnect USB

### Monitor Serial Output

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
Status: Mode=1 Pattern=0 Track=0 Step=0 Tempo=120 BPM
```

**If You See Warnings:**

```
WARNING: SD card initialization failed!
```
→ Check: SD card inserted? Formatted FAT32? Try reinserting.

```
WARNING: No Lua modes found on SD card
```
→ Check: `modes/` directory exists on SD? .lua files present?

---

## Step 7: Test MIDI Output

### Connect to DAW

1. **Windows:** Install Teensy USB MIDI driver (auto-detected)
2. **Mac/Linux:** Teensy shows up as "Teensy MIDI" automatically

### In Your DAW

1. Create MIDI track
2. Select input: "Teensy MIDI"
3. Load a synth (any instrument)
4. You should see **MIDI Clock** messages immediately

### Test with Desktop Simulator First

If you have buttons/pots wired, press them! Otherwise, test with desktop simulator first:

```bash
# On desktop
bin/gruvbok

# Create pattern with buttons
# Turn Mode knob to select drum mode (1)
# Press buttons B1-B8 to program drum hits
# Save as song
```

Then load that song onto SD card.

---

## Step 8: Connect Hardware (Optional)

### Wiring Guide

See `src/teensy/README.md` for complete pin mappings.

**Quick Reference:**
```
Buttons B1-B16:  Pins 0-15 (INPUT_PULLUP, connect to GND)
Rotary R1-R4:    Analog A0-A3 (10kΩ pots, center to pin, ends to 3.3V/GND)
Slider S1-S4:    Analog A4-A7 (10kΩ pots, same as rotary)
LED:             Pin 13 (built-in LED, or external with 220Ω resistor)
```

### Test Buttons

Press any button → Serial monitor should show event changes.

### Test Pots

Turn any pot → Mode/Tempo/Pattern/Track should change.

---

## Troubleshooting

### Build Errors

**"lua.h: No such file or directory"**
- Solution: Did you copy Lua source to `lib/lua/`? See Step 2.

**"undefined reference to `lua_*`"**
- Solution: Lua source files missing or incomplete.

**Memory exceeds 1MB**
```
DATA: [==========] 110% (used 1.1MB out of 1MB)
```
- Solution: Reduce number of Lua modes on SD card to 6-8 modes
- Or: Enable additional optimizations in `lib/lua/library.json`

### Upload Errors

**"Teensy not found"**
- Press physical button on Teensy
- Try different USB port/cable
- Reinstall Teensy drivers

**"Permission denied" (Linux)**
```bash
sudo usermod -a -G dialout $USER
# Log out and back in
```

### Runtime Errors

**No MIDI output**
- Check: Is playback started? (Serial says "Playback started"?)
- Check: Are modes loaded? (Serial says "Successfully loaded N modes")
- Check: Are there events programmed? (Use desktop to create patterns first)

**LED doesn't blink**
- LED should blink every quarter note (4 steps)
- Verify tempo is reasonable (60-240 BPM)
- Check: Is engine playing? (Serial says "Playback started")

**Buttons don't respond**
- Wiring: Buttons connect pin to GND when pressed (INPUT_PULLUP)
- Check pin numbers match BUTTON_PINS in teensy_hardware.h

---

## Memory Optimization

If you hit memory limits, try these:

### Reduce Number of Modes

Only put 6-8 modes on SD card instead of all 15:
```
SD:/modes/
  00_song.lua    (essential)
  01_drums.lua
  02_acid.lua
  03_chords.lua
  04_arpeggiator.lua
  05_euclidean.lua
```

### Use Compiled Lua Bytecode (Advanced)

Compile Lua scripts to bytecode on desktop:
```bash
luac -o 01_drums.luac modes/01_drums.lua
```

Modify ModeLoader to load `.luac` files instead of `.lua`.
Bytecode is smaller and loads faster.

### Disable Unused Lua Libraries

Edit `lib/lua/linit.c` and comment out libraries you don't need:
```c
// {"io", luaopen_io},     // File I/O not needed
// {"os", luaopen_os},     // OS functions not needed
```

---

## Next Steps

### Create Custom Modes

1. Copy `modes/TEMPLATE.lua` to `modes/08_mymode.lua`
2. Edit on desktop, test with `bin/gruvbok`
3. Copy to SD card
4. Power cycle Teensy to reload modes

### Save/Load Songs (Future)

Song save/load to SD card is partially implemented. See Song::save() and Song::load() in `src/core/song.cpp`.

To enable:
1. Add "Save" and "Load" button functions
2. Write JSON files to `SD:/songs/`
3. Add UI for file selection (button combinations)

### Build Enclosure

GRUVBOK is designed as a desktop/tabletop groovebox. Consider:
- 3D-printed case for Teensy + buttons/pots
- Laser-cut acrylic faceplate
- MIDI DIN output (in addition to USB)
- External power supply (Teensy can run on 5V USB or 3.3-5.5V on Vin)

---

## Reference

- **Project Docs:** `CLAUDE.md`, `docs/LUA_API.md`
- **Teensy 4.1:** https://www.pjrc.com/store/teensy41.html
- **PlatformIO:** https://docs.platformio.org/en/latest/platforms/teensy.html
- **Lua Manual:** https://www.lua.org/manual/5.4/

---

## Success Checklist

- [ ] Lua source files in `lib/lua/` (Step 2)
- [ ] SD card formatted FAT32 (Step 3)
- [ ] Modes copied to `SD:/modes/` (Step 3)
- [ ] Firmware builds successfully (Step 4)
- [ ] Memory usage < 800KB (Step 4)
- [ ] Firmware uploaded to Teensy (Step 5)
- [ ] SD card inserted into Teensy (Step 6)
- [ ] Serial monitor shows "Successfully loaded N modes" (Step 6)
- [ ] LED blinks on beat (Step 7)
- [ ] MIDI Clock visible in DAW (Step 7)
- [ ] Buttons/pots respond (Step 8, optional)

**You now have a working hardware GRUVBOK groovebox!** 🎹🎛️🎉

---

## Support

- Issues: GitHub Issues
- Architecture questions: See `CLAUDE.md`
- Lua API: See `docs/LUA_API.md`
- Hardware: See `src/teensy/README.md`
