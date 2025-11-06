#!/bin/bash
set -e

echo "=== GRUVBOK Build Script ==="
echo

# Check for dependencies
echo "Checking dependencies..."

if ! command -v cmake &> /dev/null; then
    echo "ERROR: cmake not found. Install with: brew install cmake"
    exit 1
fi

if ! command -v lua &> /dev/null; then
    echo "ERROR: lua not found. Install with: brew install lua"
    exit 1
fi

# Check for RtMidi (try to find via brew first)
RTMIDI_FOUND=0
if command -v brew &> /dev/null; then
    BREW_PREFIX=$(brew --prefix 2>/dev/null)
    if [ -f "$BREW_PREFIX/lib/librtmidi.dylib" ]; then
        RTMIDI_FOUND=1
    fi
fi

# Fall back to standard locations
if [ $RTMIDI_FOUND -eq 0 ]; then
    if [ -f "/opt/homebrew/lib/librtmidi.dylib" ] || [ -f "/usr/local/lib/librtmidi.dylib" ]; then
        RTMIDI_FOUND=1
    fi
fi

if [ $RTMIDI_FOUND -eq 0 ]; then
    echo "ERROR: RtMidi not found. Install with: brew install rtmidi"
    echo
    echo "After installing, verify with:"
    echo "  brew list rtmidi"
    echo "  brew info rtmidi"
    exit 1
fi

echo "✓ All dependencies found"
echo

# Create build directory
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

# Configure
echo "Configuring with CMake..."
cmake .. || {
    echo
    echo "ERROR: CMake configuration failed"
    echo "Try manually specifying paths:"
    echo "  cmake .. -DLUA_INCLUDE_DIR=/opt/homebrew/include/lua -DLUA_LIBRARIES=/opt/homebrew/lib/liblua.dylib"
    exit 1
}

echo
echo "Building..."
make -j$(sysctl -n hw.ncpu) || {
    echo
    echo "ERROR: Build failed"
    exit 1
}

echo
echo "=== Build Complete! ==="
echo
echo "To run GRUVBOK:"
echo "  ./build/bin/gruvbok"
echo
echo "(Make sure to run from the project root directory)"
echo
