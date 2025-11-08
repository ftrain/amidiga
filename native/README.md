# GRUVBOK Native (macOS & iOS)

Native SwiftUI versions of GRUVBOK for macOS and iOS, built on the same C++ engine as the desktop version.

## Architecture

```
┌─────────────────────────────────────┐
│   SwiftUI (macOS/iOS)               │
│   - ContentView, KnobView, Grid     │
│   - Platform-specific apps          │
└──────────────┬──────────────────────┘
               │ Swift ↔ Objective-C++
┌──────────────▼──────────────────────┐
│   Bridge (Objective-C++)            │
│   - EngineWrapper.mm                │
│   - EngineState.swift               │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│   C++ Core (100% shared)            │
│   - Song, Engine, MidiScheduler     │
│   - Lua modes, Pattern data         │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│   Hardware Layer                    │
│   - MacOSHardware.mm (CoreMIDI)     │
│   - IOSHardware.mm (AVAudioEngine)  │
└─────────────────────────────────────┘
```

## Features

### macOS
- ✅ Native CoreMIDI support (virtual MIDI port)
- ✅ AVAudioEngine for internal audio synthesis
- ✅ SwiftUI interface with rotary knobs
- ✅ 16-button pattern grid (click to toggle)
- ✅ Real-time Lua mode execution
- ✅ Song save/load (JSON format)
- ✅ Multi-timbral playback (15 modes simultaneously)

### iOS
- ✅ AVAudioEngine for internal audio (primary)
- ✅ Touch-optimized UI with gesture controls
- ✅ Haptic feedback on button presses and tempo beats
- ✅ Same C++ engine as desktop/macOS
- ✅ Portable groovebox in your pocket
- ⚠️ CoreMIDI optional (requires hardware interface)

## Directory Structure

```
native/
├── Shared/
│   ├── Views/
│   │   ├── ContentView.swift          # Main UI (macOS & iOS)
│   │   ├── KnobView.swift             # Rotary control
│   │   └── PatternGridView.swift      # 16-step grid
│   ├── Bridge/
│   │   ├── EngineWrapper.h/mm         # C++ ↔ Swift bridge
│   │   └── EngineState.swift          # ObservableObject
│   └── Resources/
│       └── modes/ -> ../../modes      # Lua scripts (symlink)
├── macOS/
│   ├── GruvbokApp.swift               # macOS app entry
│   └── MacOSHardware.h/mm             # CoreMIDI + AVAudioEngine
├── iOS/
│   ├── GruvbokApp.swift               # iOS app entry
│   └── IOSHardware.h/mm               # Touch + haptics
├── Core -> ../src/core                # Symlink to C++ core
├── LuaBridge -> ../src/lua_bridge     # Symlink to Lua integration
└── Hardware -> ../src/hardware        # Symlink to hardware interface
```

## Building in Xcode

### Step 1: Create Xcode Project

1. Open Xcode
2. **File → New → Project**
3. Choose **Multiplatform → App**
4. Product Name: **GRUVBOK**
5. Interface: **SwiftUI**
6. Language: **Swift**
7. Save to: `native/` directory

### Step 2: Add Files to Xcode

**Add Shared Files:**
- Drag `Shared/Views/*.swift` → "Shared" group
- Drag `Shared/Bridge/*.swift` → "Shared" group
- Drag `Shared/Bridge/*.h` and `*.mm` → "Shared" group

**Add Platform Files:**
- Drag `macOS/GruvbokApp.swift` → "macOS" target
- Drag `macOS/MacOSHardware.h/mm` → "macOS" target
- Drag `iOS/GruvbokApp.swift` → "iOS" target
- Drag `iOS/IOSHardware.h/mm` → "iOS" target

**Add C++ Core:**
- Drag `Core/` (symlink) → Both targets
- Drag `LuaBridge/` (symlink) → Both targets
- Drag `Hardware/` (symlink) → Both targets

**Add Lua Modes:**
- Drag `Shared/Resources/modes/` → Both targets
- Check "Copy Bundle Resources" in Build Phases

### Step 3: Configure Build Settings

**Both Targets (macOS & iOS):**

1. **Build Settings → Apple Clang - Language:**
   - C++ Language Dialect: `C++17 [-std=c++17]`
   - C++ Standard Library: `libc++ (LLVM C++ standard library)`

2. **Build Settings → Search Paths:**
   - Header Search Paths:
     ```
     $(SRCROOT)/native
     $(SRCROOT)/src
     /usr/local/include
     ```
     (Add `/opt/homebrew/include` on Apple Silicon Macs)

3. **Build Settings → Linking:**
   - Other Linker Flags: `-llua`
   - Library Search Paths:
     ```
     /usr/local/lib
     /opt/homebrew/lib
     ```

4. **Build Phases → Link Binary With Libraries:**
   - macOS: Add `CoreMIDI.framework`, `AVFoundation.framework`, `CoreAudio.framework`
   - iOS: Add `CoreMIDI.framework`, `AVFoundation.framework`, `UIKit.framework`

5. **Build Settings → Packaging:**
   - Info.plist File: Use generated plist (or create custom)

**Objective-C++ Bridging:**
- Xcode auto-detects `.mm` files and enables Obj-C++ bridging
- Create bridging header if needed: **File → New → Header File**
  - Name: `GRUVBOK-Bridging-Header.h`
  - Import: `#import "EngineWrapper.h"`
  - Add to Build Settings → Swift Compiler - General → Objective-C Bridging Header

### Step 4: Install Lua (if not already installed)

**macOS:**
```bash
# Homebrew
brew install lua

# Or MacPorts
sudo port install lua
```

**iOS:**
- Use static Lua library (compile for arm64)
- Or use CocoaPods: `pod 'Lua'`

### Step 5: Build & Run

1. Select target: **GRUVBOK (macOS)** or **GRUVBOK (iOS)**
2. Press **Cmd+R** to build and run
3. First launch will initialize audio and load Lua modes

## Using the App

### macOS

**Controls:**
- **Mode/Tempo/Pattern/Track knobs**: Drag up/down to adjust
- **S1-S4 sliders**: Move sliders to set parameter values
- **Pattern grid**: Click buttons 1-16 to toggle events
  - **Green**: Active event
  - **Red**: Currently playing step
  - **Gray**: Empty step
- **Hold button**: Hold and move sliders to update event parameters in real-time

**Audio:**
- Internal audio uses AVAudioEngine with built-in DLS synth
- External MIDI outputs to "GRUVBOK Virtual" port (use IAC Driver or connect to DAW)

### iOS

**Controls:**
- **Knobs**: Drag up/down (vertical gesture)
- **Sliders**: Tap and drag (rotated for mobile)
- **Pattern grid**: Tap to toggle events
- **Haptic feedback**: Feel the beat and button presses

**Audio:**
- Prioritizes internal audio (AVAudioEngine)
- Screen stays awake during use
- Background audio supported

## Troubleshooting

### Build Errors

**"Cannot find 'lua' in scope"**
- Ensure Lua is installed: `brew install lua`
- Add Lua include path to Header Search Paths
- Add `-llua` to Other Linker Flags

**"Undefined symbols for architecture arm64"**
- Check that all `.mm` and `.cpp` files are added to Compile Sources
- Verify C++ Standard Library is set to `libc++`

**"No such file or directory: 'CoreMIDI/CoreMIDI.h'"**
- Add `CoreMIDI.framework` to Link Binary With Libraries
- macOS only - not needed for simulator

### Runtime Issues

**"No audio output"**
- Check that `default.sf2` is in bundle Resources (or use built-in DLS)
- Verify audio session is active (iOS)
- Check System Preferences → Sound → Output (macOS)

**"Lua modes not loading"**
- Verify `modes/` folder is in Bundle Resources
- Check console for "Loaded X modes from..." message
- Ensure `.lua` files are UTF-8 encoded

**"MIDI not working (macOS)"**
- Open Audio MIDI Setup app
- Enable IAC Driver (Window → Show MIDI Studio)
- Connect virtual port to IAC Bus 1

## Performance Notes

### macOS
- Target: 60fps UI, <5ms MIDI jitter
- Lua GC: Optimized for real-time (no pauses observed)
- Memory: ~20MB (song data) + ~10MB (Lua states)

### iOS
- Target: 60fps UI, haptic feedback <10ms latency
- Audio latency: ~20ms (acceptable for programming, not live performance)
- Battery: Audio engine is power-efficient, can run for hours

## Next Steps

1. **Add SoundFont**: Drop a `.sf2` file into project and add to Bundle Resources
2. **Customize UI**: Edit SwiftUI views for your aesthetic
3. **More Modes**: Create new Lua modes in `modes/` directory
4. **Distribute**: Archive and notarize for distribution (macOS) or submit to App Store (iOS)

## Comparison: Native vs SDL Desktop

| Feature | Native (macOS/iOS) | SDL Desktop |
|---------|-------------------|-------------|
| **Look & Feel** | Native, platform-specific | Cross-platform (same on all OS) |
| **Audio** | AVAudioEngine (low latency) | FluidSynth + SDL2 audio |
| **MIDI** | CoreMIDI (native) | RtMidi (wrapper) |
| **Binary Size** | ~5MB | ~15MB (includes SDL2) |
| **Performance** | Optimized for platform | Good, slightly higher overhead |
| **Development** | Xcode + SwiftUI | CMake + C++ |
| **Distribution** | App Store ready | GitHub releases, manual install |

## Resources

- **SwiftUI**: https://developer.apple.com/documentation/swiftui/
- **CoreMIDI**: https://developer.apple.com/documentation/coremidi
- **AVAudioEngine**: https://developer.apple.com/documentation/avfaudio/avaudioengine
- **Lua C API**: https://www.lua.org/manual/5.4/manual.html#4

## License

Same as GRUVBOK desktop version (see parent README.md)
