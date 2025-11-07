#!/usr/bin/env python3
"""
PlatformIO pre-build script to automatically download and embed Lua 5.4.6 source.

This script:
1. Checks if lib/lua already contains Lua source
2. Downloads Lua 5.4.6 from lua.org if needed
3. Extracts and copies source files to lib/lua
4. Removes standalone interpreters (lua.c, luac.c, onelua.c)
5. Creates library.json for PlatformIO
6. Cleans up temporary files

Usage: Runs automatically via platformio.ini extra_scripts directive
       Can also be run standalone: python scripts/embed_lua.py
"""

import os
import urllib.request
import tarfile
import shutil
import json
import sys

# Try to import PlatformIO environment (only available when run from PlatformIO)
try:
    Import("env")
    RUNNING_IN_PLATFORMIO = True
    project_dir = env.get("PROJECT_DIR")
except:
    RUNNING_IN_PLATFORMIO = False
    # When running standalone, use current directory or parent
    project_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Configuration
LUA_VERSION = "5.4.6"
LUA_URL = f"http://www.lua.org/ftp/lua-{LUA_VERSION}.tar.gz"
LUA_DIR = "lib/lua"
LUA_TARBALL = f"lua-{LUA_VERSION}.tar.gz"
LUA_EXTRACT_DIR = f"lua-{LUA_VERSION}"

# Files to exclude from build
EXCLUDE_FILES = ["lua.c", "luac.c", "onelua.c"]
lua_lib_path = os.path.join(project_dir, LUA_DIR)
tarball_path = os.path.join(project_dir, LUA_TARBALL)
extract_path = os.path.join(project_dir, LUA_EXTRACT_DIR)


def check_lua_exists():
    """Check if Lua source files already exist."""
    if not os.path.exists(lua_lib_path):
        return False

    # Check for key Lua files
    required_files = ["lua.h", "lapi.c", "lstate.c"]
    for file in required_files:
        if not os.path.exists(os.path.join(lua_lib_path, file)):
            return False

    print(f"✓ Lua source already exists in {LUA_DIR}")
    return True


def download_lua():
    """Download Lua tarball from lua.org using multiple methods."""
    print(f"Downloading Lua {LUA_VERSION} from {LUA_URL}...")

    # Method 1: Try with curl (most reliable)
    try:
        import subprocess
        result = subprocess.run(
            ['curl', '-L', '-o', tarball_path, LUA_URL],
            capture_output=True,
            timeout=60
        )
        if result.returncode == 0 and os.path.exists(tarball_path):
            print(f"✓ Downloaded {LUA_TARBALL} (via curl)")
            return True
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass  # curl not available or timeout, try next method

    # Method 2: Try with wget
    try:
        import subprocess
        result = subprocess.run(
            ['wget', '-O', tarball_path, LUA_URL],
            capture_output=True,
            timeout=60
        )
        if result.returncode == 0 and os.path.exists(tarball_path):
            print(f"✓ Downloaded {LUA_TARBALL} (via wget)")
            return True
    except (FileNotFoundError, subprocess.TimeoutExpired):
        pass  # wget not available or timeout, try next method

    # Method 3: Try urllib with User-Agent
    try:
        req = urllib.request.Request(
            LUA_URL,
            headers={'User-Agent': 'Mozilla/5.0 (PlatformIO Lua Embedder)'}
        )
        with urllib.request.urlopen(req, timeout=60) as response:
            with open(tarball_path, 'wb') as f:
                shutil.copyfileobj(response, f)
        print(f"✓ Downloaded {LUA_TARBALL} (via urllib)")
        return True
    except Exception as e:
        print(f"  ✗ urllib failed: {e}")

    print(f"✗ Failed to download Lua from {LUA_URL}")
    print(f"  Please manually download and extract to {LUA_DIR}/")
    print(f"  Or check your internet connection")
    return False


def extract_lua():
    """Extract Lua tarball."""
    print(f"Extracting {LUA_TARBALL}...")
    try:
        with tarfile.open(tarball_path, "r:gz") as tar:
            tar.extractall(project_dir)
        print(f"✓ Extracted to {LUA_EXTRACT_DIR}")
        return True
    except Exception as e:
        print(f"✗ Failed to extract Lua: {e}")
        return False


def copy_lua_sources():
    """Copy Lua source files to lib/lua and exclude standalone interpreters."""
    print(f"Copying Lua source files to {LUA_DIR}...")

    # Create lib/lua if it doesn't exist
    os.makedirs(lua_lib_path, exist_ok=True)

    src_dir = os.path.join(extract_path, "src")
    copied_count = 0
    excluded_count = 0

    # Copy all .c and .h files
    for file in os.listdir(src_dir):
        if file.endswith((".c", ".h")):
            # Skip excluded files
            if file in EXCLUDE_FILES:
                excluded_count += 1
                print(f"  ✗ Excluded: {file}")
                continue

            src_file = os.path.join(src_dir, file)
            dst_file = os.path.join(lua_lib_path, file)
            shutil.copy2(src_file, dst_file)
            copied_count += 1

    print(f"✓ Copied {copied_count} source files ({excluded_count} excluded)")
    return True


def create_library_json():
    """Create library.json for PlatformIO."""
    library_config = {
        "name": "lua",
        "version": LUA_VERSION,
        "description": "Lua 5.4.6 embedded for Teensy (optimized build)",
        "keywords": "lua, scripting, embedded",
        "license": "MIT",
        "homepage": "https://www.lua.org",
        "build": {
            "flags": [
                "-DLUA_32BITS",      # Use 32-bit integers (saves memory)
                "-DLUA_USE_C89",     # C89 compatibility for embedded systems
                "-DLUA_USE_POSIX"    # POSIX features
            ],
            "srcFilter": [
                "+<*.c>",
                "-<lua.c>",          # Exclude standalone interpreter
                "-<luac.c>",         # Exclude compiler
                "-<onelua.c>"        # Exclude amalgamation
            ]
        }
    }

    library_json_path = os.path.join(lua_lib_path, "library.json")
    with open(library_json_path, "w") as f:
        json.dump(library_config, f, indent=2)

    print(f"✓ Created {os.path.join(LUA_DIR, 'library.json')}")
    return True


def cleanup():
    """Remove temporary files."""
    print("Cleaning up temporary files...")

    # Remove tarball
    if os.path.exists(tarball_path):
        os.remove(tarball_path)
        print(f"  ✓ Removed {LUA_TARBALL}")

    # Remove extraction directory
    if os.path.exists(extract_path):
        shutil.rmtree(extract_path)
        print(f"  ✓ Removed {LUA_EXTRACT_DIR}/")

    print("✓ Cleanup complete")


def exit_with_error(message):
    """Exit with error, handling both PlatformIO and standalone modes."""
    print(f"✗ {message}")
    if RUNNING_IN_PLATFORMIO:
        env.Exit(1)
    else:
        sys.exit(1)


def main():
    """Main entry point for the script."""
    print("=" * 60)
    print("Lua 5.4.6 Embedding Script for Teensy")
    print("=" * 60)

    # Check if Lua already exists
    if check_lua_exists():
        print("Lua is ready. Skipping download.")
        return

    # Download and setup Lua
    print(f"\nLua source not found. Setting up Lua {LUA_VERSION}...")

    if not download_lua():
        exit_with_error("Failed to download Lua. Build may fail.")
        return

    if not extract_lua():
        exit_with_error("Failed to extract Lua. Build may fail.")
        return

    if not copy_lua_sources():
        exit_with_error("Failed to copy Lua sources. Build may fail.")
        return

    if not create_library_json():
        exit_with_error("Failed to create library.json. Build may fail.")
        return

    cleanup()

    print("\n" + "=" * 60)
    print(f"✓ Lua {LUA_VERSION} is ready for Teensy build!")
    print("=" * 60)


# Run the script
if __name__ == "__main__":
    main()
else:
    # Called from PlatformIO
    main()
