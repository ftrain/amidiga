# GRUVBOK Native - Quick Start Guide

Get the native macOS or iOS app running in 15 minutes.

## Prerequisites

- **macOS 13+** or **iOS 16+**
- **Xcode 15+**
- **Lua 5.4** (macOS only, for linking)

## Install Lua (macOS)

```bash
brew install lua
```

Verify:
```bash
lua -v
# Expected: Lua 5.4.x
```

## Create Xcode Project (One-Time Setup)

### Option A: Manual Xcode Project Creation (Recommended)

1. **Open Xcode**

2. **File → New → Project**

3. **Choose template:**
   - Platform: **Multiplatform**
   - Type: **App**
   - Click **Next**

4. **Configure project:**
   - Product Name: `GRUVBOK`
   - Team: (Select your team)
   - Organization Identifier: `com.yourname.gruvbok`
   - Interface: **SwiftUI**
   - Language: **Swift**
   - Click **Next**

5. **Save location:**
   - Navigate to `amidiga/native/`
   - Click **Create**

6. **Delete default files:**
   - Delete `ContentView.swift` (we have our own)
   - Delete default app file (we have platform-specific ones)

### Step 2: Add Source Files to Xcode

**Drag these into Xcode's file navigator:**

1. **Shared Target** (both macOS & iOS):
   - `Shared/Views/` → Drag entire folder
   - `Shared/Bridge/EngineWrapper.h` and `.mm`
   - `Shared/Bridge/EngineState.swift`
   - Symlinks: `Core/`, `LuaBridge/`, `Hardware/`

2. **macOS Target ONLY**:
   - `macOS/GruvbokApp.swift`
   - `macOS/MacOSHardware.h` and `.mm`

3. **iOS Target ONLY**:
   - `iOS/GruvbokApp.swift`
   - `iOS/IOSHardware.h` and `.mm`

4. **Resources** (both targets):
   - `Shared/Resources/modes/` → Check "Copy Bundle Resources"

**Important:** When adding symlinks (Core, LuaBridge, Hardware), Xcode will follow them and add the actual source files.

### Step 3: Configure Build Settings

**For BOTH macOS and iOS targets:**

1. **Select target** in Xcode (left sidebar)

2. **Build Settings tab** → Search for "C++ Language"
   - **C++ Language Dialect**: `GNU++17 [-std=gnu++17]`
   - **C++ Standard Library**: `libc++`

3. **Build Settings** → Search for "Header Search"
   - **Header Search Paths**: Add these (recursive):
     ```
     $(SRCROOT)
     $(SRCROOT)/..
     /opt/homebrew/include
     /usr/local/include
     ```

4. **Build Settings** → Search for "Library Search"
   - **Library Search Paths**: Add:
     ```
     /opt/homebrew/lib
     /usr/local/lib
     ```

5. **Build Settings** → Search for "Other Linker"
   - **Other Linker Flags**: Add:
     ```
     -llua
     ```

6. **Build Phases tab** → **Link Binary With Libraries** → Click **+**
   - Add: `CoreMIDI.framework`
   - Add: `AVFoundation.framework`
   - Add: `CoreAudio.framework` (macOS only)
   - Add: `UIKit.framework` (iOS only - should already be there)

### Step 4: Create Bridging Header (Objective-C++ ↔ Swift)

1. **File → New → File**
2. Choose **Header File**
3. Name: `GRUVBOK-Bridging-Header.h`
4. Save in `native/` directory
5. Add this content:

```objc
//
//  GRUVBOK-Bridging-Header.h
//

#ifndef GRUVBOK_Bridging_Header_h
#define GRUVBOK_Bridging_Header_h

#import "Shared/Bridge/EngineWrapper.h"

#endif /* GRUVBOK_Bridging_Header_h */
```

6. **Build Settings** → Search for "Bridging Header"
   - **Objective-C Bridging Header**: `$(SRCROOT)/GRUVBOK-Bridging-Header.h`

### Step 5: Configure Info.plist (Optional but Recommended)

**macOS:**
- Allow microphone access (if using MIDI input): `NSMicrophoneUsageDescription`

**iOS:**
- Background audio: Add "audio" to `UIBackgroundModes`
- Keep screen awake: Handled in code (`UIApplication.shared.isIdleTimerDisabled = true`)

### Step 6: Build & Run

1. **Select target**:
   - macOS: "GRUVBOK (My Mac)" or "GRUVBOK (Mac Catalyst)"
   - iOS: "GRUVBOK (iOS Simulator)" or connected device

2. **Press Cmd+B** to build (check for errors)

3. **Press Cmd+R** to run

**First Launch:**
- Console will show: "Loaded X modes from..."
- Audio engine will initialize
- UI will appear with knobs and pattern grid

## Verify It Works

1. **Check LED**: Green circle should blink on tempo beat
2. **Click pattern grid buttons**: Should toggle green/gray
3. **Drag Mode knob**: Should change mode number
4. **Drag Tempo knob**: Beat should speed up/slow down
5. **Listen**: If audio is working, you should hear demo pattern (drums + acid bass)

## Troubleshooting

### "Cannot find 'lua' in scope" (Build Error)

**Solution:**
```bash
brew install lua
```
Then restart Xcode and rebuild.

### "Symbol not found: _lua_..." (Link Error)

**Solution:**
- Add `-llua` to Other Linker Flags
- Add `/opt/homebrew/lib` to Library Search Paths

### "No audio output"

**Solution (macOS):**
1. Check System Preferences → Sound → Output
2. Verify Audio MIDI Setup → IAC Driver is enabled
3. Try restarting the app

**Solution (iOS):**
1. Check device volume
2. Check silent mode switch
3. Try connecting headphones

### "Lua modes not loading"

**Solution:**
1. Check Build Phases → Copy Bundle Resources
2. Verify `modes/` folder is included
3. Check console for "WARNING: modes/ directory not found"

### iOS Simulator: "Haptic feedback not working"

**Expected:** Haptic feedback only works on real devices, not simulator.

## Next Steps

- **Try all modes**: Change Mode knob (0-14)
- **Create patterns**: Use pattern grid to program beats
- **Edit Lua modes**: Open `modes/*.lua` files in Xcode and customize
- **Read main README.md** for architecture details

## Files You Need

Minimal file list to double-check:

```
native/
├── GRUVBOK.xcodeproj/           # Created by Xcode
├── GRUVBOK-Bridging-Header.h    # You create this
├── Shared/
│   ├── Views/
│   │   ├── ContentView.swift
│   │   ├── KnobView.swift
│   │   └── PatternGridView.swift
│   └── Bridge/
│       ├── EngineWrapper.h
│       ├── EngineWrapper.mm
│       └── EngineState.swift
├── macOS/
│   ├── GruvbokApp.swift
│   ├── MacOSHardware.h
│   └── MacOSHardware.mm
└── iOS/
    ├── GruvbokApp.swift
    ├── IOSHardware.h
    └── IOSHardware.mm
```

Plus symlinks: `Core/`, `LuaBridge/`, `Hardware/` → `../src/`

## Getting Help

- Check Xcode console for error messages
- Read main `README.md` for detailed docs
- Ensure all build settings match this guide

Good luck! 🎵
