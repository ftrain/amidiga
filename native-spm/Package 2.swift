// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "GRUVBOK",
    platforms: [
        .macOS(.v13)
    ],
    products: [
        .executable(
            name: "gruvbok-native",
            targets: ["GRUVBOK"])
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
                .linkedLibrary("lua")
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
                .headerSearchPath("../../src"),
                .define("TARGET_OS_OSX", to: "1")
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
            dependencies: ["GRUVBOKBridge", "GRUVBOKCore"],
            path: "Sources/GRUVBOK",
            sources: [
                "App.swift",
                "Views/ContentView.swift",
                "Views/KnobView.swift",
                "Views/PatternGridView.swift",
                "Bridge/EngineState.swift"
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
