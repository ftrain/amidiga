#!/usr/bin/env python3
"""
PlatformIO pre-build script to patch Lua 5.4 for Teensy compatibility.
This script modifies luaconf.h to enable LUA_32BITS mode.
"""

Import("env")
import os

def patch_lua_config(*args, **kwargs):
    """Patch Lua's luaconf.h to enable 32-bit mode for embedded systems"""

    # Find the lua library directory
    lib_dir = os.path.join(env.subst("$PROJECT_DIR"), ".pio", "libdeps", env.subst("$PIOENV"), "lua")
    luaconf_path = os.path.join(lib_dir, "luaconf.h")

    if not os.path.exists(luaconf_path):
        print(f"⚠️  Lua not yet downloaded, will patch on next build")
        return

    # Read the file
    with open(luaconf_path, 'r') as f:
        content = f.read()

    # Check if already patched
    if "PATCHED FOR TEENSY" in content:
        print("✓ Lua already patched for Teensy")
        # Still need to remove problematic files on every build
        remove_lua_duplicates(lib_dir)
        return

    # Patch: Replace line that defines LUA_32BITS to 0
    # with a conditional definition that respects command-line flags
    original_line = "#define LUA_32BITS\t0"
    replacement = """/* PATCHED FOR TEENSY: Allow command-line override */
#if !defined(LUA_32BITS)
#define LUA_32BITS\t0
#endif"""

    if original_line in content:
        content = content.replace(original_line, replacement)

        # Write back
        with open(luaconf_path, 'w') as f:
            f.write(content)

        print("✓ Patched luaconf.h for Teensy 32-bit mode")
    else:
        print(f"⚠️  Could not find expected LUA_32BITS definition in {luaconf_path}")
        print("    The Lua library may have changed. Manual patching required.")

    # Remove duplicate source files
    remove_lua_duplicates(lib_dir)

def remove_lua_duplicates(lib_dir):
    """Remove Lua amalgamation and standalone interpreter files to avoid multiple definitions"""
    files_to_remove = ['onelua.c', 'lua.c', 'luac.c']

    for filename in files_to_remove:
        filepath = os.path.join(lib_dir, filename)
        if os.path.exists(filepath):
            try:
                os.remove(filepath)
                print(f"✓ Removed {filename} (avoiding duplicate symbols)")
            except Exception as e:
                print(f"⚠️  Could not remove {filename}: {e}")


# Register the callback to run before building any library
# This ensures Lua is patched before the library build starts
env.AddPreAction("buildprog", patch_lua_config)
