#!/bin/bash

echo "=== GRUVBOK Dependency Diagnostic ==="
echo

# Check Homebrew
echo "1. Checking Homebrew..."
if command -v brew &> /dev/null; then
    BREW_PREFIX=$(brew --prefix)
    echo "   ✓ Homebrew found at: $BREW_PREFIX"
else
    echo "   ✗ Homebrew NOT found"
    echo "   Install from: https://brew.sh"
    exit 1
fi
echo

# Check CMake
echo "2. Checking CMake..."
if command -v cmake &> /dev/null; then
    CMAKE_VERSION=$(cmake --version | head -1)
    echo "   ✓ $CMAKE_VERSION"
else
    echo "   ✗ CMake NOT found"
    echo "   Install with: brew install cmake"
fi
echo

# Check Lua
echo "3. Checking Lua..."
if command -v lua &> /dev/null; then
    LUA_VERSION=$(lua -v 2>&1)
    echo "   ✓ $LUA_VERSION"

    # Check for Lua library
    if [ -f "$BREW_PREFIX/lib/liblua.dylib" ] || [ -f "$BREW_PREFIX/lib/liblua.a" ]; then
        echo "   ✓ Lua library found"
    else
        echo "   ⚠ Lua library might not be found"
    fi
else
    echo "   ✗ Lua NOT found"
    echo "   Install with: brew install lua"
fi
echo

# Check RtMidi
echo "4. Checking RtMidi..."
if brew list rtmidi &> /dev/null; then
    echo "   ✓ RtMidi package installed via Homebrew"

    # Check header file
    if [ -f "$BREW_PREFIX/include/RtMidi.h" ]; then
        echo "   ✓ Header found: $BREW_PREFIX/include/RtMidi.h"
    else
        echo "   ✗ Header NOT found at: $BREW_PREFIX/include/RtMidi.h"
        echo "   Expected locations:"
        find /usr/local/include /opt/homebrew/include -name "RtMidi.h" 2>/dev/null | sed 's/^/     /'
    fi

    # Check library file
    if [ -f "$BREW_PREFIX/lib/librtmidi.dylib" ]; then
        echo "   ✓ Library found: $BREW_PREFIX/lib/librtmidi.dylib"
        ls -lh "$BREW_PREFIX/lib/librtmidi.dylib" | awk '{print "     Size: " $5 ", Date: " $6 " " $7 " " $8}'
    else
        echo "   ✗ Library NOT found at: $BREW_PREFIX/lib/librtmidi.dylib"
        echo "   Expected locations:"
        find /usr/local/lib /opt/homebrew/lib -name "librtmidi*" 2>/dev/null | sed 's/^/     /'
    fi

    # Check pkg-config
    if pkg-config --exists rtmidi 2>/dev/null; then
        echo "   ✓ pkg-config can find rtmidi"
        echo "     Version: $(pkg-config --modversion rtmidi)"
        echo "     Cflags: $(pkg-config --cflags rtmidi)"
        echo "     Libs: $(pkg-config --libs rtmidi)"
    else
        echo "   ⚠ pkg-config cannot find rtmidi (this is OK)"
    fi
else
    echo "   ✗ RtMidi NOT installed"
    echo "   Install with: brew install rtmidi"
fi
echo

# Summary
echo "=== Summary ==="
ALL_OK=true

if ! command -v cmake &> /dev/null; then
    echo "✗ Missing: CMake - Install with: brew install cmake"
    ALL_OK=false
fi

if ! command -v lua &> /dev/null; then
    echo "✗ Missing: Lua - Install with: brew install lua"
    ALL_OK=false
fi

if ! brew list rtmidi &> /dev/null 2>&1; then
    echo "✗ Missing: RtMidi - Install with: brew install rtmidi"
    ALL_OK=false
fi

if [ "$ALL_OK" = true ]; then
    echo "✓ All dependencies are installed!"
    echo
    echo "If CMake still can't find RtMidi, try:"
    echo "  cd build && rm -rf *"
    echo "  cmake .. -DRTMIDI_INCLUDE_DIRS=$BREW_PREFIX/include -DRTMIDI_LIBRARIES=$BREW_PREFIX/lib/librtmidi.dylib"
    echo "  make"
else
    echo
    echo "Install missing dependencies and run this script again."
fi
echo
