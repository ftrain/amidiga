# GRUVBOK macOS Build Guide

Complete guide for building the native macOS app in Xcode.

## Prerequisites

1. **macOS 13+** (Ventura or later)
2. **Xcode 15+**
3. **Lua 5.4** installed via Homebrew

```bash
brew install lua
lua -v  # Should show: Lua 5.4.x
```

## Project Structure (After Cleanup)

```
native/GRUVBOK/
├── GRUVBOK.xcodeproj/          # Xcode project file
├── GRUVBOK-Bridging-Header.h   # Objective-C++ ↔ Swift bridge
├── Assets.xcassets/            # App icons (optional)
│
├── GRUVBOK/                    # App target (shared)
│   ├── GRUVBOKApp.swift        # UNUSED (placeholder only)
│   └── modes/ -> ../../../modes  # Symlink to Lua modes
│
├── Shared/                     # Cross-platform code
│   ├── Bridge/
│   │   ├── EngineWrapper.h
│   │   ├── EngineWrapper.mm    # C++ ↔ Objective-C++
│   │   └── EngineState.swift   # Swift ObservableObject
│   └── Views/
│       ├── ContentView.swift   # Main UI (adaptive)
│       ├── KnobView.swift
│       └── PatternGridView.swift
│
├── macOS/                      # macOS-specific
│   ├── GruvbokApp.swift        # **ACTUAL** @main entry point
│   ├── MacOSHardware.h
│   └── MacOSHardware.mm        # CoreMIDI + AVAudioEngine
│
└── iOS/                        # iOS-specific
    ├── GruvbokApp.swift        # iOS @main entry point
    ├── IOSHardware.h
    └── IOSHardware.mm
```

## Opening the Project in Xcode

### Method 1: Open Existing Project (Recommended)

The Xcode project already exists at `native/GRUVBOK/GRUVBOK.xcodeproj`.

```bash
cd amidiga/native/GRUVBOK
open GRUVBOK.xcodeproj
```

Xcode will open. **You may see errors immediately** - this is expected. Continue to configuration steps below.

### Method 2: If Xcode Project is Corrupted

If the project won't open or Xcode crashes:

1. Rename the broken project:
   ```bash
   mv GRUVBOK.xcodeproj GRUVBOK.xcodeproj.backup
   ```

2. Create a new project:
   - Open Xcode
   - File → New → Project
   - Platform: **macOS** (NOT Multiplatform)
   - Type: **App**
   - Product Name: `GRUVBOK`
   - Interface: **SwiftUI**
   - Language: **Swift**
   - Save to: `amidiga/native/` (NOT inside GRUVBOK/)

3. Continue to configuration steps below

## Configuration Steps

### Step 1: Remove Xcode Template Files

The default Xcode project includes files we don't need:

1. In Xcode's file navigator (left sidebar), select and **delete**:
   - `ContentView.swift` (if in GRUVBOK/ target) - we have our own in `Shared/Views/`
   - Any auto-generated `GRUVBOKApp.swift` that imports SwiftData
   - `Item.swift` (SwiftData model we don't use)

**Keep:**
- `Assets.xcassets`
- `GRUVBOK-Bridging-Header.h`

### Step 2: Add Source Files to Xcode Project

**Important:** Drag files from Finder into Xcode's file navigator. Check the "Add to targets" boxes carefully!

#### 2a. Add Shared Files (Both macOS AND iOS targets)

Right-click on `GRUVBOK` project → Add Files to "GRUVBOK"...

Navigate to `native/GRUVBOK/Shared/` and add:
- `Bridge/EngineWrapper.h` ✓ macOS ✓ iOS
- `Bridge/EngineWrapper.mm` ✓ macOS ✓ iOS
- `Bridge/EngineState.swift` ✓ macOS ✓ iOS
- `Views/ContentView.swift` ✓ macOS ✓ iOS
- `Views/KnobView.swift` ✓ macOS ✓ iOS
- `Views/PatternGridView.swift` ✓ macOS ✓ iOS

**Options when adding:**
- ✓ Copy items if needed: **NO** (use references)
- Create groups: **Yes**
- Add to targets: **CHECK BOTH macOS AND iOS**

#### 2b. Add macOS-Specific Files (macOS target ONLY)

Navigate to `native/GRUVBOK/macOS/` and add:
- `GruvbokApp.swift` ✓ macOS **ONLY**
- `MacOSHardware.h` ✓ macOS **ONLY**
- `MacOSHardware.mm` ✓ macOS **ONLY**

**Options:**
- Add to targets: **CHECK macOS ONLY** (uncheck iOS)

#### 2c. Add iOS-Specific Files (iOS target ONLY)

Navigate to `native/GRUVBOK/iOS/` and add:
- `GruvbokApp.swift` ✓ iOS **ONLY**
- `IOSHardware.h` ✓ iOS **ONLY**
- `IOSHardware.mm` ✓ iOS **ONLY**

**Options:**
- Add to targets: **CHECK iOS ONLY** (uncheck macOS)

#### 2d. Add C++ Core Code (Both targets)

The C++ engine code is in the parent `src/` directory. Add these files to the project:

Navigate to `src/core/` and add ALL `.h` and `.cpp` files:
- `event.h`, `event.cpp`
- `pattern.h`, `pattern.cpp`
- `song.h`, `song.cpp`
- `engine.h`, `engine.cpp`

Navigate to `src/hardware/` and add:
- `hardware_interface.h`
- `midi_scheduler.h`, `midi_scheduler.cpp`
- `audio_output.h`, `audio_output.cpp`

Navigate to `src/lua_bridge/` and add:
- `lua_context.h`, `lua_context.cpp`
- `lua_api.h`, `lua_api.cpp`
- `mode_loader.h`, `mode_loader.cpp`

**Options:**
- Add to targets: **CHECK BOTH macOS AND iOS**

#### 2e. Add Lua Modes as Resources

Navigate to `modes/` directory and **select all `.lua` files**.

**Options:**
- Copy items if needed: **YES** (copy into bundle)
- Create folder references: **Yes**
- Add to targets: **CHECK BOTH**

**Verify:** In Build Phases → Copy Bundle Resources, you should see `modes/` folder listed.

### Step 3: Configure Build Settings

Select **GRUVBOK** target (macOS) in the left sidebar, then **Build Settings** tab.

**Enable "All" and "Combined" filters** at the top to see all settings.

#### 3a. C++ Language Settings

Search for "C++ Language":
- **C++ Language Dialect**: `GNU++17 [-std=gnu++17]` or `C++17 [-std=c++17]`
- **C++ Standard Library**: `libc++ (LLVM C++ standard library)`

#### 3b. Header Search Paths

Search for "Header Search":
- **Header Search Paths**: Add these paths (all **recursive**):
  ```
  $(SRCROOT)
  $(SRCROOT)/..
  $(SRCROOT)/../..
  /opt/homebrew/include
  /usr/local/include
  ```

  **Note:** Different Mac architectures use different paths:
  - Apple Silicon (M1/M2): `/opt/homebrew/include`
  - Intel Macs: `/usr/local/include`

  Add **both** to be safe.

#### 3c. Library Search Paths

Search for "Library Search":
- **Library Search Paths**: Add:
  ```
  /opt/homebrew/lib
  /usr/local/lib
  ```

#### 3d. Other Linker Flags

Search for "Other Linker":
- **Other Linker Flags**: Add:
  ```
  -llua
  ```

  **Important:** This must be exactly `-llua`, not `-llua5.4` or any variant.

#### 3e. Bridging Header

Search for "Bridging":
- **Objective-C Bridging Header**:
  ```
  $(SRCROOT)/GRUVBOK-Bridging-Header.h
  ```

  **Or if that doesn't work:**
  ```
  GRUVBOK-Bridging-Header.h
  ```

### Step 4: Link Frameworks

Select target → **Build Phases** tab → **Link Binary With Libraries** → Click **+**

Add these frameworks:
- `CoreMIDI.framework`
- `AVFoundation.framework`
- `CoreAudio.framework`
- `Accelerate.framework` (for audio processing)

**Do NOT add:**
- SwiftData
- UIKit (macOS target)

### Step 5: Verify Bridging Header Content

Open `GRUVBOK-Bridging-Header.h` and ensure it contains:

```objc
//
//  GRUVBOK-Bridging-Header.h
//

#ifndef GRUVBOK_Bridging_Header_h
#define GRUVBOK_Bridging_Header_h

#import "Shared/Bridge/EngineWrapper.h"

#endif /* GRUVBOK_Bridging_Header_h */
```

**Path must match exactly** - if your files are organized differently, update the path.

### Step 6: Fix Compile Sources

Go to target → **Build Phases** → **Compile Sources**

**Verify these files are listed:**
- All `*.cpp` files from `src/core/`, `src/hardware/`, `src/lua_bridge/`
- `EngineWrapper.mm`
- `MacOSHardware.mm` (macOS target only)
- `IOSHardware.mm` (iOS target only)
- All `*.swift` files

**Remove if present:**
- Duplicate files
- Test files (unless you want to run tests)

### Step 7: Set Deployment Target

Search for "Deployment":
- **macOS Deployment Target**: `13.0` or later
- **iOS Deployment Target**: `16.0` or later

### Step 8: Build!

1. Select target: **GRUVBOK (My Mac)** from the scheme dropdown
2. Press **Cmd+B** to build

#### Expected Build Output

```
Build target GRUVBOK
Compiling event.cpp
Compiling pattern.cpp
Compiling song.cpp
Compiling engine.cpp
...
Linking GRUVBOK
Build succeeded
```

#### Common Build Errors & Fixes

**Error: "Undefined symbols for architecture arm64: _lua_..."**
- Fix: Add `-llua` to Other Linker Flags
- Fix: Add `/opt/homebrew/lib` to Library Search Paths

**Error: "Cannot find 'EngineWrapper' in scope"**
- Fix: Check Bridging Header path in Build Settings
- Fix: Ensure `EngineWrapper.h` is in the project

**Error: "No such file or directory: 'lua.h'"**
- Fix: Add Lua include path to Header Search Paths: `/opt/homebrew/include`
- Fix: Install Lua: `brew install lua`

**Error: "Use of undeclared identifier 'Song'"**
- Fix: Add all C++ core files to Compile Sources

**Error: "Multiple @main entry points"**
- Fix: Ensure only ONE file has `@main` attribute per target
- Fix: Remove `@main` from `GRUVBOK/GRUVBOKApp.swift` (it should not have it)
- Fix: Keep `@main` ONLY in `macOS/GruvbokApp.swift` for macOS target

**Warning: "ContentView initializer requires 'platform' argument"**
- Fix: Ensure you're using `macOS/GruvbokApp.swift` which calls `ContentView(platform: "macOS")`
- Fix: Do NOT use the auto-generated GRUVBOKApp.swift

### Step 9: Run the App

1. Press **Cmd+R** to build and run
2. The app window should appear with:
   - Title: "GRUVBOK"
   - Four rotary knobs (Mode, Tempo, Pattern, Track)
   - Four sliders (S1-S4)
   - 16-button pattern grid
   - LED tempo indicator (green circle)
   - Save/load buttons
   - "Audio Ready" indicator (green)

3. **Test the app:**
   - Click buttons in the pattern grid → should toggle green/gray
   - Drag Mode knob → mode number should change
   - Drag Tempo knob → tempo should change, LED blink rate should adjust
   - Listen for audio (drums + bass on default pattern)

## Troubleshooting

### Xcode Crashes on Open

If Xcode crashes when opening the project:

1. **Clear Xcode derived data:**
   ```bash
   rm -rf ~/Library/Developer/Xcode/DerivedData/*
   ```

2. **Try opening from Terminal:**
   ```bash
   open -a Xcode native/GRUVBOK/GRUVBOK.xcodeproj
   ```

3. **If still crashing:** Create a new project (see "Method 2" above)

### No Audio Output

1. **Check Audio MIDI Setup:**
   - Open Audio MIDI Setup app (in /Applications/Utilities/)
   - Window → Show MIDI Studio
   - Enable "IAC Driver" if using external MIDI

2. **Check System Sound:**
   - System Preferences → Sound → Output
   - Ensure output is not muted
   - Try with headphones

3. **Check Console for errors:**
   - In Xcode, check the console output when app runs
   - Look for "Audio engine initialized" message

### Lua Modes Not Loading

1. **Check Bundle Resources:**
   - Target → Build Phases → Copy Bundle Resources
   - Verify `modes/` folder is listed

2. **Check console:**
   - Look for "Loaded X modes from..."
   - If 0 modes loaded, check paths

3. **Rebuild:**
   - Product → Clean Build Folder (Cmd+Shift+K)
   - Product → Build (Cmd+B)

## Teensy Compatibility

All changes are compatible with Teensy:
- Lua mode changes (SLIDER_LABELS) are optional metadata
- C++ core changes (isDirty, clearDirty, markDirty) are standard C++
- Native app changes are macOS/iOS-only (SwiftUI layer)

The same C++ code compiles for:
- Desktop (Linux/Mac/Windows with SDL2 + ImGui)
- Native (macOS/iOS with SwiftUI)
- Teensy 4.1 (embedded, no UI)

## Next Steps

- **Customize UI:** Edit `Shared/Views/ContentView.swift`
- **Add modes:** Create new `.lua` files in `modes/`
- **Modify hardware:** Edit `macOS/MacOSHardware.mm` for audio/MIDI changes
- **Distribute:** Archive and notarize for distribution

## Summary Checklist

- [ ] Lua installed (`brew install lua`)
- [ ] Xcode project opens without crashing
- [ ] All source files added to project
- [ ] Build settings configured (C++17, Header Search Paths, Linker Flags)
- [ ] Frameworks linked (CoreMIDI, AVFoundation, CoreAudio)
- [ ] Bridging header path set correctly
- [ ] Only ONE @main per target (in macOS/GruvbokApp.swift)
- [ ] Project builds successfully (Cmd+B)
- [ ] App runs and shows UI (Cmd+R)
- [ ] Audio works
- [ ] Pattern grid responds to clicks

**If all boxes are checked, you're ready to make music!** 🎵
