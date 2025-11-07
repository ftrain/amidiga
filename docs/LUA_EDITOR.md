# Lua Mode Editor

## Overview

GRUVBOK includes a built-in Lua editor for live coding and mode development. Edit modes in real-time, hot-reload them without restarting, and (on Teensy) save directly to SD card.

## Desktop Editor Features

### Main Features
- **Mode selector** - Choose which mode (0-14) to edit
- **Load from Disk** - Open existing mode files from `modes/` directory
- **Save to Disk** - Write changes back to file
- **Hot Reload Mode** - Instantly apply changes to the running engine
- **Template generation** - Auto-create template for new modes
- **32KB code buffer** - Plenty of space for complex Lua scripts
- **Tab support** - Use Tab key for indentation

### GUI Layout

**Location:** Separate "Lua Mode Editor" window (right side of screen by default)

**Controls:**
- **Mode dropdown** - Select Mode 0-14
- **Load from Disk** - Reload current file
- **Save to Disk** - Save changes
- **Hot Reload Mode** - Save + reload + reinit
- **File path** - Shows current file being edited
- **Large text editor** - Full-screen multiline editor with Tab support

### Workflow

1. **Select a mode** from dropdown (e.g., "Mode 1 (Drums)")
2. **Edit the Lua code** directly in the editor
3. **Click "Hot Reload Mode"** to apply changes instantly
4. **Hear the results** immediately in playback!

### Hot Reload

When you click "Hot Reload Mode":
1. Saves current buffer to disk
2. Reloads Lua file into engine
3. Calls `init(context)` with current tempo
4. Mode starts using new code immediately
5. No need to stop/restart playback!

### File Naming

Modes are stored in `modes/` directory:
```
modes/00_song.lua
modes/01_drums.lua
modes/02_acid.lua
modes/03_chords.lua
modes/04_mode4.lua  (if you create mode 4)
...
modes/14_mode14.lua
```

### Template Mode

If you select a mode that doesn't exist yet, the editor auto-generates a template:

```lua
-- GRUVBOK Lua Mode Template
-- Mode: N

function init(context)
    -- Called once when mode loads
    -- context.tempo = BPM
    -- context.mode_number = 0-14
    -- context.midi_channel = 0-14
end

function process_event(track, event)
    -- Called for each event during playback
    -- track: 0-7
    -- event.switch: true/false
    -- event.pots: {s1, s2, s3, s4} (0-127 each)

    if event.switch then
        -- Send MIDI note
        note(60, 100)  -- Middle C, velocity 100
        off(60, 100)   -- Note off after 100ms
    end
end
```

Just fill in your custom logic and hot-reload!

---

## Teensy SD Card Support

### Overview

On Teensy 4.1, Lua modes will be stored on the built-in SD card slot. This allows:
- **Portable mode libraries** - Swap SD cards to change your mode collection
- **Persistent storage** - Modes survive power cycles
- **Field updates** - Edit modes on desktop, copy to SD card
- **Larger mode capacity** - SD card has way more space than internal flash

### Hardware Requirements

**Teensy 4.1 only** - The Teensy 4.1 has a built-in microSD card slot. Teensy 4.0 does not.

**SD Card:**
- MicroSD or MicroSDHC (up to 32GB)
- FAT32 formatted
- Class 10 recommended (for faster load times)

### SD Card Layout

```
/gruvbok/
├── modes/
│   ├── 00_song.lua
│   ├── 01_drums.lua
│   ├── 02_acid.lua
│   ├── 03_chords.lua
│   └── ... (up to 14_*.lua)
├── songs/
│   ├── my_song_001.json
│   └── ... (saved songs)
└── config.ini (optional)
```

### Teensy Implementation

#### SdFat Library

GRUVBOK uses **SdFat** library for SD card access:
```cpp
#include <SdFat.h>

SdFat SD;
File luaFile;
```

#### Boot Sequence (Teensy)

1. Initialize SD card
2. Check for `/gruvbok/modes/` directory
3. Load all 15 mode files (00-14)
4. If a mode file is missing, use built-in default
5. Start playback engine

#### Mode Loading (Teensy)

```cpp
bool ModeLoader::loadFromSD(int mode_num) {
    char filename[64];
    snprintf(filename, sizeof(filename), "/gruvbok/modes/%02d_*.lua", mode_num);

    // Try to open file
    File file = SD.open(filename, FILE_READ);
    if (!file) {
        Serial.printf("Mode %d not found on SD, using default\n", mode_num);
        return loadBuiltInDefault(mode_num);
    }

    // Read file into buffer
    size_t file_size = file.size();
    if (file_size > MAX_LUA_FILE_SIZE) {
        Serial.println("Error: Lua file too large");
        file.close();
        return false;
    }

    char* buffer = new char[file_size + 1];
    file.read(buffer, file_size);
    buffer[file_size] = '\0';
    file.close();

    // Load into Lua
    LuaContext* lua = getLuaContext(mode_num);
    bool success = lua->loadString(buffer);
    delete[] buffer;

    return success;
}
```

#### Saving Modes (Teensy)

```cpp
bool ModeLoader::saveToSD(int mode_num, const char* lua_code) {
    char filename[64];
    snprintf(filename, sizeof(filename), "/gruvbok/modes/%02d_mode.lua", mode_num);

    // Open file for writing
    File file = SD.open(filename, FILE_WRITE | O_TRUNC);
    if (!file) {
        Serial.println("Error: Failed to open file for writing");
        return false;
    }

    // Write Lua code
    size_t written = file.write((const uint8_t*)lua_code, strlen(lua_code));
    file.close();

    return (written == strlen(lua_code));
}
```

### Desktop → Teensy Workflow

#### Option 1: Direct SD Card Copy (Easiest)

1. **Edit modes on desktop** using built-in editor
2. **Save changes** (writes to `modes/` directory)
3. **Eject SD card** from Teensy
4. **Insert into computer** (USB SD card reader)
5. **Copy modes to SD card:**
   ```bash
   cp modes/*.lua /Volumes/GRUVBOK/gruvbok/modes/
   ```
6. **Eject and reinsert** into Teensy
7. **Power cycle Teensy** - modes auto-load!

#### Option 2: USB Serial Transfer (Advanced)

Future feature: Send Lua code over USB serial

1. Desktop GUI connects to Teensy via USB
2. Click "Send to Teensy" button
3. Lua code transferred over serial
4. Teensy saves to SD card automatically
5. Hot-reload on device

**Commands:**
```
SAVE_MODE <mode_num> <length>\n
<lua_code_here>
\n
LOAD_MODE <mode_num>\n
RELOAD_MODE <mode_num>\n
LIST_MODES\n
```

### Error Handling (Teensy)

**SD card not present:**
- Fall back to built-in default modes (compiled into firmware)
- Show error on LED (fast blinking pattern)
- Continue playback with defaults

**Corrupt Lua file:**
- Log error to serial
- Use built-in default for that mode
- Other modes continue working

**Write failure:**
- Retry once
- If still fails, log error
- Keep old file intact

### Performance Considerations

**Load time:**
- 15 Lua files × ~2KB average = 30KB total
- SD card read: ~500 KB/s
- Total load time: < 100ms

**Caching:**
- Lua files loaded once at boot
- Stay in RAM during playback
- No disk I/O during performance

**Hot-reload on Teensy:**
- Read file from SD
- Parse Lua
- Swap out old mode
- ~50-100ms interruption (acceptable)

---

## Lua API Reference

For complete Lua API documentation, see `docs/LUA_API.md`.

### Quick Reference

**MIDI Output:**
```lua
note(pitch, velocity, delta)  -- Note on (+ optional delay)
off(pitch, delta)              -- Note off
cc(controller, value, delta)   -- Control change
stopall(delta)                 -- All notes off
```

**LED Control:**
```lua
led_pattern(pattern_name, brightness)
-- Patterns: "tempo_beat", "button_held", "saving", "loading", "error"
```

**Context (init only):**
```lua
function init(context)
    local bpm = context.tempo
    local mode = context.mode_number
    local chan = context.midi_channel
end
```

**Event Data (process_event):**
```lua
function process_event(track, event)
    local button_pressed = event.switch  -- true/false
    local slider1 = event.pots[1]        -- 0-127
    local slider2 = event.pots[2]        -- 0-127
    local slider3 = event.pots[3]        -- 0-127
    local slider4 = event.pots[4]        -- 0-127
end
```

---

## Tips & Tricks

### Live Coding Workflow

1. **Start with a template** - Use Mode 4+ for experiments
2. **Test incrementally** - Small changes, frequent hot-reloads
3. **Use System Log** - Watch for Lua errors
4. **Comment your code** - You'll thank yourself later
5. **Save often** - Desktop auto-saves, but be safe

### Common Patterns

**Velocity from slider:**
```lua
if event.switch then
    local vel = event.pots[1]  -- S1 controls velocity
    note(60, vel)
    off(60, 100)
end
```

**Pitch from slider:**
```lua
if event.switch then
    local pitch = 36 + math.floor(event.pots[1] * 48 / 127)  -- C1 to B4
    note(pitch, 100)
    off(pitch, 100)
end
```

**Per-track notes (drum machine):**
```lua
local drum_notes = {36, 38, 42, 46, 49, 51, 53, 56}  -- GM drums
function process_event(track, event)
    if event.switch then
        local drum = drum_notes[track + 1]
        note(drum, event.pots[1])  -- Velocity from S1
        off(drum, 10)
    end
end
```

**CC control:**
```lua
if event.switch then
    note(60, 100)
    off(60, 100)
end

-- Always send filter cutoff (CC 74) from S4
cc(74, event.pots[4])
```

### Debugging

**Print to System Log:**
```lua
-- Lua print() goes to desktop System Log
print("Debug: track=" .. track .. " vel=" .. event.pots[1])
```

**Test on desktop first:**
- Faster iteration
- Better error messages
- No risk to hardware

**Check syntax:**
```bash
lua -c modes/01_drums.lua
# If no output = valid syntax
```

---

## Troubleshooting

### Desktop Editor

**"Failed to load" error:**
- Check file exists in `modes/` directory
- Verify file permissions (readable)
- Check filename matches pattern: `XX_name.lua`

**Hot-reload doesn't work:**
- Check System Log for Lua syntax errors
- Verify `init()` and `process_event()` functions exist
- Make sure mode_loader is initialized

**Code not saving:**
- Check write permissions on `modes/` directory
- Verify disk space available
- Check filename is valid

### Teensy SD Card

**Modes not loading:**
- Verify SD card is FAT32 formatted
- Check `/gruvbok/modes/` directory exists
- Verify filenames are correct: `00_song.lua`, etc.
- Check serial output for error messages

**SD card not detected:**
- Reseat SD card
- Try different SD card (some brands more reliable)
- Verify Teensy 4.1 (4.0 has no SD slot)
- Check SdFat library installed

**Write failures:**
- SD card may be write-protected (check physical switch)
- Card may be full (check free space)
- Card may be corrupt (reformat as FAT32)

---

## Future Enhancements

- [ ] **Syntax highlighting** - Color-code Lua keywords
- [ ] **Line numbers** - Easier debugging
- [ ] **Find/Replace** - Search within code
- [ ] **Undo/Redo** - Multi-level undo
- [ ] **USB serial transfer** - Send modes to Teensy without SD card removal
- [ ] **Mode library browser** - Browse and load community modes
- [ ] **Template gallery** - Pre-made mode templates
- [ ] **Lua error highlighting** - Show syntax errors inline
- [ ] **Auto-complete** - Suggest GRUVBOK API functions

---

## Example: Creating a New Mode

Let's create a **arpeggiator mode** from scratch:

1. **Open Lua Mode Editor**
2. **Select "Mode 4"** from dropdown
3. **Click "Load from Disk"** - Template appears
4. **Replace template with:**

```lua
-- GRUVBOK Mode 4: Arpeggiator
-- S1 = Root note (MIDI)
-- S2 = Arp pattern (0-7)
-- S3 = Octave range (1-4)
-- S4 = Note length (ms)

local patterns = {
    {0, 4, 7},           -- Major chord
    {0, 3, 7},           -- Minor chord
    {0, 4, 7, 12},       -- Major + octave
    {0, 3, 7, 12},       -- Minor + octave
    {0, 7, 12, 19},      -- Fifth progression
    {0, 2, 4, 7, 9, 11}, -- Major scale
    {0, 2, 3, 7, 8, 10}, -- Minor scale
    {0, 12, 0, 12}       -- Octave bounce
}

function init(context)
    -- Nothing to initialize
end

function process_event(track, event)
    if event.switch then
        local root = math.floor(event.pots[1] * 84 / 127) + 24  -- C1 to C7
        local pattern_idx = math.floor(event.pots[2] * 7 / 127) + 1
        local octaves = math.floor(event.pots[3] * 3 / 127) + 1
        local length = math.floor(event.pots[4] * 480 / 127) + 20  -- 20-500ms

        local pattern = patterns[pattern_idx]
        local note_idx = (track % #pattern) + 1
        local octave_offset = math.floor(track / #pattern) * 12

        if octave_offset < (octaves * 12) then
            local pitch = root + pattern[note_idx] + octave_offset
            note(pitch, 100)
            off(pitch, length)
        end
    end
end
```

5. **Click "Hot Reload Mode"**
6. **Set R1 (Mode) to 4**
7. **Program some steps** with B1-B16
8. **Adjust sliders** S1-S4 to control arpeggio
9. **Hear it play!**

---

**GRUVBOK Lua Editor: Code while you play!** 🎹💻🎛️
