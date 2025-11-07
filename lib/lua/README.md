# Lua 5.4.6 for Teensy 4.1

This directory contains the Lua 5.4.6 library configured for Teensy 4.1.

## Quick Setup

### Option 1: Download Lua Source (Recommended)

```bash
# Download Lua 5.4.6
cd lib/lua
curl -R -O http://www.lua.org/ftp/lua-5.4.6.tar.gz
tar zxf lua-5.4.6.tar.gz

# Copy source files (not lua.c or luac.c - those are standalone interpreters)
cp lua-5.4.6/src/*.c .
cp lua-5.4.6/src/*.h .

# Clean up
rm -rf lua-5.4.6 lua-5.4.6.tar.gz
```

After this, you should have all .c and .h files in `lib/lua/`.

### Option 2: Use PlatformIO Library (If Available)

Check if there's a pre-packaged Lua library:
```bash
pio lib search lua
```

## Memory Optimization Flags

The `library.json` file includes important flags for Teensy:

### `-DLUA_32BITS`
- **What it does:** Uses 32-bit integers instead of 64-bit
- **Memory saved:** ~30-40% reduction in Lua state size
- **Impact:** Lua numbers limited to 32-bit range (still plenty for MIDI/audio)

### `-DLUA_USE_C89`
- **What it does:** Uses C89 standard (more portable)
- **Benefit:** Better compatibility with Arduino/Teensy toolchain

### `-DLUA_COMPAT_5_3`
- **What it does:** Enable Lua 5.3 compatibility features
- **Benefit:** More Lua scripts work without modification

## Expected Memory Usage

With these optimizations:

```
Single Lua context (1 mode): ~40-60 KB
8 Lua contexts (8 modes):    ~320-480 KB  ✅ SAFE
15 Lua contexts (15 modes):  ~600-900 KB  ⚠️  TIGHT (may exceed 1MB limit)
```

**Recommendation:** Start with 8 modes, expand if memory allows.

## Files Needed

After extracting Lua source, you should have:

```
lib/lua/
├── lapi.c
├── lapi.h
├── lauxlib.c
├── lauxlib.h
├── lbaselib.c
├── lcode.c
├── lcode.h
├── lcorolib.c
├── lctype.c
├── lctype.h
├── ldblib.c
├── ldebug.c
├── ldebug.h
├── ldo.c
├── ldo.h
├── ldump.c
├── lfunc.c
├── lfunc.h
├── lgc.c
├── lgc.h
├── linit.c
├── liolib.c
├── llex.c
├── llex.h
├── lmathlib.c
├── lmem.c
├── lmem.h
├── loadlib.c
├── lobject.c
├── lobject.h
├── lopcodes.c
├── lopcodes.h
├── loslib.c
├── lparser.c
├── lparser.h
├── lstate.c
├── lstate.h
├── lstring.c
├── lstring.h
├── lstrlib.c
├── ltable.c
├── ltable.h
├── ltablib.c
├── ltm.c
├── ltm.h
├── lua.h
├── luaconf.h
├── lualib.h
├── lundump.c
├── lundump.h
├── lutf8lib.c
├── lvm.c
├── lvm.h
├── lzio.c
└── lzio.h
```

**DO NOT include:**
- `lua.c` (standalone Lua interpreter)
- `luac.c` (Lua compiler)
- `onelua.c` (single-file Lua - we use modular build)

## Testing

After setting up Lua source:

```bash
# Test compilation
platformio run -e teensy41

# Check memory usage
pio run -e teensy41 -t size
```

## Troubleshooting

### "lua.h: No such file or directory"
- Make sure all .c and .h files are in `lib/lua/`
- Check that `library.json` exists

### "undefined reference to `__imp___iob_func`"
- This is a Windows-specific issue
- Solution: Use `-DLUA_USE_C89` flag (already in library.json)

### Memory exceeded (>1MB RAM)
- Reduce number of modes in ModeLoader
- Consider using Lua bytecode instead of source
- Enable additional optimizations in luaconf.h

## Next Steps

1. Get Lua source files into this directory
2. Build with `platformio run -e teensy41`
3. Check memory with `pio run -e teensy41 -t size`
4. If memory OK, proceed to SD card integration

## References

- Lua 5.4 Reference: https://www.lua.org/manual/5.4/
- PlatformIO Teensy: https://docs.platformio.org/en/latest/platforms/teensy.html
- Teensy 4.1 Specs: https://www.pjrc.com/store/teensy41.html
