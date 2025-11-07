# Migration to LuaArduino Library

**Date:** 2025-11-07
**Issue:** Teensy 4.1 build failed with RAM1 overflow (needed 592KB, only 512KB available)
**Solution:** Switched from full Lua 5.4.6 to LuaArduino library (Lua 5.1 with ~80KB footprint)

---

## Changes Made

### 1. Updated `platformio.ini`

**Before:**
```ini
lib_deps =
    ; Lua will be compiled from source (see lib/ directory)

src_dir = src/teensy
lib_dir = lib
```

**After:**
```ini
lib_deps =
    blackketter/LuaArduino @ ^1.0.0

build_src_filter =
    +<*>
    +<../core/*>
    +<../hardware/*>
    +<../lua_bridge/*>
    -<../desktop/*>

build_flags =
    -DLUA_OK=0  ; Lua 5.1 compatibility
```

### 2. Compatibility Fix

Added `-DLUA_OK=0` to build flags because:
- Lua 5.4 defines `LUA_OK` constant (value 0)
- Lua 5.1 (used by LuaArduino) doesn't have this constant
- Our code uses `LUA_OK` in 3 places (all in `lua_context.cpp`)

---

## Lua 5.4 → 5.1 Compatibility

✅ **All API functions we use are compatible:**

| Function | Lua 5.1 | Lua 5.4 | Status |
|----------|---------|---------|--------|
| `luaL_newstate()` | ✅ | ✅ | Compatible |
| `luaL_openlibs()` | ✅ | ✅ | Compatible |
| `lua_register()` | ✅ | ✅ | Compatible |
| `lua_pushinteger()` | ✅ | ✅ | Compatible |
| `lua_setfield()` | ✅ | ✅ | Compatible |
| `lua_getfield()` | ✅ | ✅ | Compatible |
| `luaL_checkinteger()` | ✅ | ✅ | Compatible |
| `lua_pcall()` | ✅ | ✅ | Compatible |
| `luaL_dofile()` | ✅ | ✅ | Compatible |
| `LUA_REGISTRYINDEX` | ✅ | ✅ | Compatible |
| `LUA_OK` constant | ❌ (= 0) | ✅ | Fixed via `-DLUA_OK=0` |

---

## Testing Checklist

Run these tests on your local machine:

### Build Test
```bash
cd ~/amidiga
pio run -e teensy41
```

**Expected:**
- ✅ Build completes successfully
- ✅ RAM1 usage < 512KB (no overflow)
- ✅ No Lua-related errors

### Memory Usage Verification
After successful build, check output:
```
teensy_size: Memory Usage on Teensy 4.1:
   RAM1: variables:XXXXX, code:XXXXXX, padding:XXXXX   free for local variables: >0
```

**Success criteria:** `free for local variables` should be **positive** (not negative like before: -94976)

### Runtime Test (if you have hardware)
1. Upload to Teensy 4.1
2. Test Mode 1 (drums) - should play drum sounds
3. Test Mode 2 (acid) - should play bassline
4. Verify all modes load without errors
5. Check serial monitor for Lua errors

---

## Known Differences (Lua 5.1 vs 5.4)

### Minor Lua Language Differences
Most scripts should work unchanged, but be aware:

| Feature | Lua 5.1 | Lua 5.4 |
|---------|---------|---------|
| Integer division `//` | ❌ | ✅ |
| Bitwise operators `&, |, ~` | ❌ | ✅ |
| `goto` statement | ❌ | ✅ |
| UTF-8 library | ❌ | ✅ |

**Impact:** Our modes don't use these features, so no changes needed.

### What Still Works
- ✅ Tables, arrays, functions
- ✅ Math operations (`+, -, *, /, %`)
- ✅ String manipulation
- ✅ Loops (`for`, `while`)
- ✅ Conditionals (`if`, `elseif`, `else`)
- ✅ Table iteration (`pairs`, `ipairs`)

---

## Rollback Plan (if needed)

If LuaArduino doesn't work, revert `platformio.ini`:

```ini
lib_deps =
    ; Lua will be compiled from source

lib_dir = lib
```

Then try Option 3 (optimize Lua 5.4 build flags) or Option 2 (switch to Wren).

---

## Expected Memory Savings

**Before (Lua 5.4.6 full build):**
- RAM1 usage: ~592 KB ❌ (overflow by 94KB)

**After (LuaArduino ~80KB):**
- RAM1 usage: ~200-300 KB ✅ (estimated)
- Savings: ~300-400 KB

---

## Next Steps

1. **Test build locally:**
   ```bash
   pio run -e teensy41
   ```

2. **If build succeeds:** Upload to Teensy and test all modes

3. **If build fails:** Review errors and adjust

4. **Report results:** Let me know the memory usage from build output

---

## Files Modified

- `platformio.ini` - Library dependency changed from local Lua to LuaArduino

## Files NOT Modified (no changes needed)

- `src/lua_bridge/*` - All Lua C API code compatible
- `modes/*.lua` - All Lua scripts compatible
- `src/core/*` - No changes
- `src/teensy/*` - No changes

---

## References

- **LuaArduino:** https://github.com/blackketter/LuaArduino
- **Confirmed working** on Teensy 4.0, 4.1, 3.6
- **Memory:** ~80KB program flash
- **Lua version:** 5.1

---

**Ready to test!** Run `pio run -e teensy41` on your local machine and check the memory usage.
