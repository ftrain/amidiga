# Build Test Instructions

**Status:** ✅ PlatformIO installed, but build cannot complete in this environment due to network restrictions.

---

## What Was Done

1. ✅ **Installed PlatformIO Core 6.1.18** in this environment
2. ✅ **Updated platformio.ini** to use LuaArduino library
3. ⚠️ **Build blocked** - Cannot download Teensy platform (HTTP access denied)

---

## Test Locally on Your Machine

### Prerequisites
```bash
# Install PlatformIO if you don't have it
pip install -U platformio
```

### Build Command
```bash
cd ~/path/to/amidiga
pio run -e teensy41
```

### What to Look For

#### ✅ **SUCCESS** - Expected Output:
```
Linking .pio/build/teensy41/firmware.elf
Checking size .pio/build/teensy41/firmware.elf
teensy_size: Memory Usage on Teensy 4.1:
   FLASH: code:XXXXX, data:XXXXX, headers:XXXX   free for files:XXXXXXX
   RAM1: variables:XXXXX, code:XXXXX, padding:XXXXX   free for local variables: [POSITIVE NUMBER]
   RAM2: variables:XXXX  free for malloc/new:XXXXXX
```

**Key metric:** `RAM1: free for local variables` should be **POSITIVE** (not -94976 like before)

#### ❌ **FAILURE** - If You Still See:
```
RAM1: variables:XXXXX, code:XXXXXX, padding:XXXXX   free for local variables:-94976
Error program exceeds memory space
```

Then we need to try Option 3 (optimize build flags) or Option 2 (switch to Wren).

---

## Expected Memory Improvement

| Metric | Before (Lua 5.4.6) | After (LuaArduino) | Savings |
|--------|-------------------|-------------------|---------|
| **Code size** | ~497 KB | ~80-150 KB | ~350 KB ✅ |
| **Variables** | ~95 KB | ~30-50 KB | ~45 KB ✅ |
| **Total RAM1** | ~592 KB ❌ | ~200-250 KB ✅ | ~350 KB ✅ |
| **Free RAM1** | **-94 KB** ❌ | **+250 KB** ✅ | **+344 KB** ✅ |

---

## Troubleshooting

### If Build Fails with Library Errors

Try cleaning and rebuilding:
```bash
pio run -e teensy41 --target clean
pio run -e teensy41
```

### If LuaArduino Library Not Found

Check library installation:
```bash
pio pkg list -e teensy41
```

Should show:
```
blackketter/LuaArduino @ ^1.0.0
```

If missing, manually install:
```bash
pio pkg install -e teensy41
```

### If You Get Lua API Errors

The `-DLUA_OK=0` flag should handle Lua 5.1 vs 5.4 compatibility. If you see errors about `LUA_OK`, verify the build flag in `platformio.ini`:
```ini
build_flags =
    -DLUA_OK=0  ; Lua 5.1 compatibility
```

---

## Alternative Testing (Without PlatformIO)

If you prefer Arduino IDE:

1. Install LuaArduino library via Library Manager
2. Copy `src/teensy/main.cpp` and other sources
3. Build for Teensy 4.1
4. Check memory usage in build output

---

## Report Results

After building locally, please report:

1. **Build success/failure**
2. **RAM1 memory usage** (from build output)
3. **Any error messages** (if build fails)

This will help determine if we need to pursue Option 3 (optimize flags) or Option 2 (switch to Wren).

---

## Files Changed

- `platformio.ini` - Updated for LuaArduino
- `MIGRATION_TO_LUAARDUINO.md` - Migration guide
- `BUILD_TEST_INSTRUCTIONS.md` - This file

---

**Next:** Test build on your local machine with network access! 🚀
