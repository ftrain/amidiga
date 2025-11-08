import SwiftUI

struct ContentView: View {
    @StateObject private var engine: EngineState
    @State private var localEventMonitor: Any?

    init(platform: String) {
        _engine = StateObject(wrappedValue: EngineState(platform: platform))
    }

    var body: some View {
        VStack(spacing: 0) {
            // Header
            headerSection
                .padding(.horizontal, 12)
                .padding(.vertical, 8)
                .background(Color(white: 0.12))

            Divider()
                .background(Color(white: 0.25))

            // Tabbed interface
            TabView {
                HardwareControlsView(engine: engine)
                    .tabItem {
                        Label("Hardware", systemImage: "slider.horizontal.3")
                    }

                OutputView(engine: engine)
                    .tabItem {
                        Label("Output", systemImage: "waveform")
                    }

                LuaManagerView(engine: engine)
                    .tabItem {
                        Label("Modes", systemImage: "doc.text")
                    }

                SystemLogView(engine: engine)
                    .tabItem {
                        Label("Log", systemImage: "list.bullet.rectangle")
                    }
            }
        }
        .background(Color.black)
        .preferredColorScheme(.dark)
        .frame(minWidth: 720, minHeight: 500)
        .onAppear {
            setupKeyboardMonitoring()
        }
        .onDisappear {
            if let monitor = localEventMonitor {
                NSEvent.removeMonitor(monitor)
            }
        }
    }

    private func setupKeyboardMonitoring() {
        // Use NSEvent local monitor (only works when app is frontmost)
        localEventMonitor = NSEvent.addLocalMonitorForEvents(matching: [.keyDown, .keyUp]) { event in
            // Don't intercept keyboard input when text editing is active
            if let firstResponder = NSApp.keyWindow?.firstResponder {
                if firstResponder is NSTextView ||
                   firstResponder is NSTextField ||
                   firstResponder is NSTextInputClient {
                    print("Text editing active, passing through key event")
                    return event // Pass through to text field
                }
            }

            guard let chars = event.characters, let char = chars.first else {
                return event
            }

            let pressed = (event.type == .keyDown)
            print("NSEvent monitor: key '\(char)', pressed: \(pressed)")

            // Handle button presses (key down only)
            if pressed {
                let buttonMap: [Character: Int] = [
                    "1": 0, "2": 1, "3": 2, "4": 3,
                    "q": 4, "w": 5, "e": 6, "r": 7,
                    "a": 8, "s": 9, "d": 10, "f": 11,
                    "z": 12, "x": 13, "c": 14, "v": 15
                ]

                if let button = buttonMap[char] {
                    engine.toggleButton(button)
                    return nil // Consume event
                }
            }

            // Handle knobs and sliders (key down only)
            if pressed {
                switch char {
                // Mode (R1)
                case "k":
                    if engine.rotaryPots[0] > 0 {
                        engine.setRotaryPot(0, value: engine.rotaryPots[0] - 1)
                    }
                    return nil
                case "l":
                    if engine.rotaryPots[0] < 127 {
                        engine.setRotaryPot(0, value: engine.rotaryPots[0] + 1)
                    }
                    return nil
                // Tempo (R2)
                case ";":
                    if engine.rotaryPots[1] > 0 {
                        engine.setRotaryPot(1, value: engine.rotaryPots[1] - 1)
                    }
                    return nil
                case "'":
                    if engine.rotaryPots[1] < 127 {
                        engine.setRotaryPot(1, value: engine.rotaryPots[1] + 1)
                    }
                    return nil
                // Pattern (R3)
                case "m":
                    if engine.rotaryPots[2] > 0 {
                        engine.setRotaryPot(2, value: engine.rotaryPots[2] - 1)
                    }
                    return nil
                case ",":
                    if engine.rotaryPots[2] < 127 {
                        engine.setRotaryPot(2, value: engine.rotaryPots[2] + 1)
                    }
                    return nil
                // Track (R4)
                case ".":
                    if engine.rotaryPots[3] > 0 {
                        engine.setRotaryPot(3, value: engine.rotaryPots[3] - 1)
                    }
                    return nil
                case "/":
                    if engine.rotaryPots[3] < 127 {
                        engine.setRotaryPot(3, value: engine.rotaryPots[3] + 1)
                    }
                    return nil
                // Sliders
                case "9":
                    if engine.sliderPots[0] > 0 {
                        engine.setSliderPot(0, value: engine.sliderPots[0] - 1)
                    }
                    return nil
                case "i":
                    if engine.sliderPots[0] < 127 {
                        engine.setSliderPot(0, value: engine.sliderPots[0] + 1)
                    }
                    return nil
                case "0":
                    if engine.sliderPots[1] > 0 {
                        engine.setSliderPot(1, value: engine.sliderPots[1] - 1)
                    }
                    return nil
                case "o":
                    if engine.sliderPots[1] < 127 {
                        engine.setSliderPot(1, value: engine.sliderPots[1] + 1)
                    }
                    return nil
                case "-":
                    if engine.sliderPots[2] > 0 {
                        engine.setSliderPot(2, value: engine.sliderPots[2] - 1)
                    }
                    return nil
                case "p":
                    if engine.sliderPots[2] < 127 {
                        engine.setSliderPot(2, value: engine.sliderPots[2] + 1)
                    }
                    return nil
                case "=":
                    if engine.sliderPots[3] > 0 {
                        engine.setSliderPot(3, value: engine.sliderPots[3] - 1)
                    }
                    return nil
                case "[":
                    if engine.sliderPots[3] < 127 {
                        engine.setSliderPot(3, value: engine.sliderPots[3] + 1)
                    }
                    return nil
                default:
                    break
                }
            }

            return event // Pass through unhandled events
        }
        print("Keyboard monitoring installed")
    }

    // MARK: - Header

    private var headerSection: some View {
        HStack(spacing: 15) {
            // LED indicator with glow
            HStack(spacing: 6) {
                Circle()
                    .fill(engine.ledOn ? Color.green : Color.gray.opacity(0.3))
                    .frame(width: 14, height: 14)
                    .shadow(color: engine.ledOn ? Color.green.opacity(0.8) : Color.clear, radius: 6)
                    .animation(.easeInOut(duration: 0.1), value: engine.ledOn)

                Text("GRUVBOK")
                    .font(.system(size: 18, weight: .heavy, design: .rounded))
                    .foregroundStyle(
                        LinearGradient(
                            colors: [.cyan, .blue],
                            startPoint: .leading,
                            endPoint: .trailing
                        )
                    )
            }

            Spacer()

            // Save status indicator
            HStack(spacing: 4) {
                Circle()
                    .fill(engine.isDirty ? Color.orange : Color.green)
                    .frame(width: 6, height: 6)
                    .shadow(color: engine.isDirty ? Color.orange.opacity(0.6) : Color.green.opacity(0.6), radius: 3)
                Text(engine.isDirty ? "UNSAVED" : "Saved")
                    .font(.system(size: 10, weight: .semibold, design: .monospaced))
                    .foregroundColor(engine.isDirty ? .orange : .green)
            }

            // Audio status
            if engine.isAudioReady {
                HStack(spacing: 4) {
                    Image(systemName: "speaker.wave.2.fill")
                        .font(.system(size: 10))
                        .foregroundColor(.green)
                    Text("Audio Ready")
                        .font(.system(size: 10, weight: .semibold, design: .monospaced))
                        .foregroundColor(.green)
                }
            }
        }
    }
}

// Preview disabled for SPM build
// #Preview("macOS") {
//     ContentView(platform: "macOS")
//         .frame(width: 900, height: 700)
// }
//
// #Preview("iOS") {
//     ContentView(platform: "iOS")
// }
