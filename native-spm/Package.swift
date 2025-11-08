// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "GRUVBOK",
    platforms: [
        .macOS(.v14)
    ],
    products: [
        .executable(
            name: "gruvbok-native",
            targets: ["GRUVBOK"])
    ],
    dependencies: [
        .package(url: "https://github.com/raspu/Highlightr.git", from: "2.1.2")
    ],
    targets: [
        // C++ Core library
        .target(
            name: "GRUVBOKCore",
            dependencies: [],
            path: "Sources/GRUVBOKCore",
            sources: [
                "src/core/engine.cpp",
                "src/core/event.cpp",
                "src/core/pattern.cpp",
                "src/core/song.cpp",
                "src/hardware/audio_output.cpp",
                "src/hardware/midi_scheduler.cpp",
                "src/lua_bridge/lua_api.cpp",
                "src/lua_bridge/lua_context.cpp",
                "src/lua_bridge/mode_loader.cpp"
            ],
            publicHeadersPath: "src",
            cxxSettings: [
                .headerSearchPath("src"),
                .define("TARGET_OS_OSX", to: "1"),
                .unsafeFlags([
                    "-I/opt/homebrew/opt/lua/include/lua5.4",
                    "-I/opt/homebrew/include",
                    "-I/usr/local/include"
                ])
            ],
            linkerSettings: [
                .linkedLibrary("lua"),
                .unsafeFlags([
                    "-L/opt/homebrew/opt/lua/lib",
                    "-L/usr/local/lib"
                ])
            ]
        ),
        // Objective-C++ Bridge
        .target(
            name: "GRUVBOKBridge",
            dependencies: ["GRUVBOKCore"],
            path: "Sources/GRUVBOK",
            sources: [
                "Bridge/EngineWrapper.mm",
                "Hardware/MacOSHardware.mm"
            ],
            publicHeadersPath: "Bridge",
            cxxSettings: [
                .headerSearchPath("../GRUVBOKCore/src"),
                .define("TARGET_OS_OSX", to: "1"),
                .unsafeFlags([
                    "-I/opt/homebrew/opt/lua/include/lua5.4"
                ])
            ],
            linkerSettings: [
                .linkedFramework("CoreMIDI"),
                .linkedFramework("AVFoundation"),
                .linkedFramework("CoreAudio")
            ]
        ),
        // Swift executable
        .executableTarget(
            name: "GRUVBOK",
            dependencies: [
                "GRUVBOKBridge",
                "GRUVBOKCore",
                .product(name: "Highlightr", package: "Highlightr")
            ],
            path: "Sources/GRUVBOK",
            exclude: [
                "Bridge/EngineWrapper.mm",
                "Bridge/EngineWrapper.h",
                "Hardware/MacOSHardware.mm",
                "Hardware/MacOSHardware.h"
            ],
            sources: [
                "App.swift",
                "Views/ContentView.swift",
                "Views/KnobView.swift",
                "Views/PatternGridView.swift",
                "Views/HardwareControlsView.swift",
                "Views/OutputView.swift",
                "Views/SongDataExplorerView.swift",
                "Views/SystemLogView.swift",
                "Views/ModeLabels.swift",
                "Views/LuaManagerView.swift",
                "Views/CodeEditorView.swift",
                "Bridge/EngineState.swift"
            ],
            resources: [
                .copy("../../Resources/default.sf2")
            ],
            swiftSettings: [
                .interoperabilityMode(.Cxx)
            ],
            linkerSettings: [
                .linkedFramework("AppKit")
            ]
        )
    ],
    cxxLanguageStandard: .cxx17
)
