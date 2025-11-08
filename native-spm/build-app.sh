#!/bin/bash

# Build script to create GRUVBOK.app bundle from Swift Package Manager project

set -e

echo "Building GRUVBOK..."

# Build the executable
swift build -c release

# Get the executable path
EXECUTABLE_PATH=$(swift build -c release --show-bin-path)/gruvbok-native

# Create app bundle structure
APP_NAME="GRUVBOK.app"
APP_BUNDLE="$APP_NAME"
CONTENTS_DIR="$APP_BUNDLE/Contents"
MACOS_DIR="$CONTENTS_DIR/MacOS"
RESOURCES_DIR="$CONTENTS_DIR/Resources"

echo "Creating app bundle structure..."
rm -rf "$APP_BUNDLE"
mkdir -p "$MACOS_DIR"
mkdir -p "$RESOURCES_DIR"

# Copy executable
echo "Copying executable..."
cp "$EXECUTABLE_PATH" "$MACOS_DIR/GRUVBOK"
chmod +x "$MACOS_DIR/GRUVBOK"

# Copy modes directory (Lua scripts)
echo "Copying Lua modes..."
MODES_SRC="../modes"
if [ -d "$MODES_SRC" ]; then
    cp -r "$MODES_SRC" "$RESOURCES_DIR/"
else
    echo "Warning: modes directory not found at $MODES_SRC"
fi

# Copy app icon
echo "Copying app icon..."
if [ -f "AppIcon.icns" ]; then
    cp "AppIcon.icns" "$RESOURCES_DIR/"
else
    echo "Warning: AppIcon.icns not found, creating it..."
    ./create-icon.sh 2>/dev/null || echo "Failed to create icon"
    if [ -f "AppIcon.icns" ]; then
        cp "AppIcon.icns" "$RESOURCES_DIR/"
    fi
fi

# Create Info.plist
echo "Creating Info.plist..."
cat > "$CONTENTS_DIR/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>GRUVBOK</string>
    <key>CFBundleIconFile</key>
    <string>AppIcon</string>
    <key>CFBundleIdentifier</key>
    <string>com.gruvbok.app</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>GRUVBOK</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>LSMinimumSystemVersion</key>
    <string>14.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
    <key>NSMainStoryboardFile</key>
    <string>Main</string>
    <key>LSUIElement</key>
    <false/>
</dict>
</plist>
EOF

echo "✅ Successfully created $APP_BUNDLE"
echo "You can now open it with: open $APP_BUNDLE"
