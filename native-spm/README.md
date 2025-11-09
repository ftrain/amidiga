# GRUVBOK Native (Swift Package Manager)

This is a Swift Package Manager version of the macOS app that you can **actually build and test**.

## Quick Start

```bash
cd native-spm
swift build
# If it builds: open Package.swift in Xcode
```

## Why This Exists

The Xcode project in `native/GRUVBOK/` keeps crashing because:
- Binary XML project files are hard to edit without Xcode
- Claude can't run xcodebuild to test changes
- Manual XML editing = lots of guessing

**This SPM package:**
- ✅ Text-based Package.swift (easy to edit)
- ✅ Works in terminal AND Xcode
- ✅ Can be tested with `swift build`
- ✅ Uses same C++ code from `../src/`

## Structure

```
native-spm/
├── Package.swift           # Build configuration
└── Sources/GRUVBOK/
    ├── App.swift          # macOS app entry point
    ├── Views/             # SwiftUI views (from native/GRUVBOK/Shared/Views)
    ├── Bridge/            # C++ ↔ Swift bridge (from native/GRUVBOK/Shared/Bridge)
    └── Hardware/          # macOS CoreMIDI (from native/GRUVBOK/macOS)
```

C++ source files are referenced from `../../src/` (not copied).

## Building

```bash
swift build                    # Build debug
swift build -c release         # Build release
swift run gruvbok-native       # Run the app
```

## Opening in Xcode

```bash
open Package.swift
```

Xcode will automatically generate a project from Package.swift.

## If It Doesn't Build

Common issues:

**Lua not found:**
```bash
brew install lua
```

**Wrong paths:**
Check Package.swift header search paths match your Lua installation.

**Missing C++ files:**
Ensure `../../src/core/`, `../../src/hardware/`, etc. exist.

## Next Steps

1. Try `swift build` - see actual errors
2. Fix any missing headers/libraries
3. Once building, open in Xcode
4. Test the UI

This is **much easier to debug** than the Xcode project because you can see real compiler output.
