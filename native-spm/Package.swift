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
        .executableTarget(
            name: "GRUVBOK",
            dependencies: [],
            path: "Sources/GRUVBOK",
            sources: [
                "App.swift",
                "Views/ContentView.swift",
                "Views/KnobView.swift",
                "Views/PatternGridView.swift",
                "Bridge/EngineWrapper.mm",
                "Bridge/EngineState.swift",
                "Hardware/MacOSHardware.mm",
                // C++ core
                "../../src/core/engine.cpp",
                "../../src/core/event.cpp",
                "../../src/core/pattern.cpp",
                "../../src/core/song.cpp",
                "../../src/hardware/audio_output.cpp",
                "../../src/hardware/midi_scheduler.cpp",
                "../../src/lua_bridge/lua_api.cpp",
                "../../src/lua_bridge/lua_context.cpp",
                "../../src/lua_bridge/mode_loader.cpp"
            ],
            publicHeadersPath: "Bridge",
            cxxSettings: [
                .headerSearchPath("../../src"),
                .headerSearchPath("/opt/homebrew/include"),
                .headerSearchPath("/usr/local/include"),
                .define("TARGET_OS_OSX", to: "1")
            ],
            swiftSettings: [
                .interoperabilityMode(.Cxx)
            ],
            linkerSettings: [
                .linkedLibrary("lua"),
                .linkedFramework("CoreMIDI"),
                .linkedFramework("AVFoundation"),
                .linkedFramework("CoreAudio"),
                .linkedFramework("AppKit")
            ]
        )
    ],
    cxxLanguageStandard: .cxx17
)
