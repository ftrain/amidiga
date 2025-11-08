import SwiftUI

struct ContentView: View {
    @StateObject private var engine: EngineState

    init(platform: String) {
        _engine = StateObject(wrappedValue: EngineState(platform: platform))
    }

    var body: some View {
        #if os(iOS)
        iosLayout
        #else
        macOSLayout
        #endif
    }

    // MARK: - macOS Layout

    private var macOSLayout: some View {
        VStack(spacing: 20) {
            headerSection

            HStack(spacing: 40) {
                // Left: Global knobs (2x2 grid)
                VStack(spacing: 20) {
                    HStack(spacing: 20) {
                        KnobView(
                            label: "Mode",
                            value: Binding(
                                get: { Int(engine.rotaryPots[0]) },
                                set: { engine.setRotaryPot(0, value: UInt8($0)) }
                            ),
                            range: 0...127,
                            displayText: "\(engine.currentMode)"
                        )

                        KnobView(
                            label: "Tempo",
                            value: Binding(
                                get: { Int(engine.rotaryPots[1]) },
                                set: { engine.setRotaryPot(1, value: UInt8($0)) }
                            ),
                            range: 0...127,
                            displayText: "\(engine.tempo) BPM"
                        )
                    }

                    HStack(spacing: 20) {
                        KnobView(
                            label: "Pattern",
                            value: Binding(
                                get: { Int(engine.rotaryPots[2]) },
                                set: { engine.setRotaryPot(2, value: UInt8($0)) }
                            ),
                            range: 0...127,
                            displayText: "\(engine.currentPattern + 1)"  // Display 1-32
                        )

                        KnobView(
                            label: "Track",
                            value: Binding(
                                get: { Int(engine.rotaryPots[3]) },
                                set: { engine.setRotaryPot(3, value: UInt8($0)) }
                            ),
                            range: 0...127,
                            displayText: "\(engine.currentTrack + 1)"  // Display 1-8
                        )
                    }
                }

                // Right: Sliders
                sliderSection
            }
            .padding()

            Divider()

            // Pattern grid
            patternGridSection

            Spacer()
        }
        .padding()
        .frame(minWidth: 800, minHeight: 600)
    }

    // MARK: - iOS Layout

    private var iosLayout: some View {
        ScrollView {
            VStack(spacing: 20) {
                headerSection

                // Global controls (horizontal scroll if needed)
                ScrollView(.horizontal, showsIndicators: false) {
                    HStack(spacing: 20) {
                        knobSection
                    }
                    .padding(.horizontal)
                }

                Divider()

                // Sliders (compact for mobile)
                sliderSection
                    .padding(.horizontal)

                Divider()

                // Pattern grid
                patternGridSection
                    .padding(.horizontal)
            }
            .padding()
        }
    }

    // MARK: - Sections

    private var headerSection: some View {
        HStack {
            // LED indicator
            HStack(spacing: 10) {
                Circle()
                    .fill(engine.ledOn ? Color.green : Color.gray.opacity(0.3))
                    .frame(width: 20, height: 20)
                    .animation(.easeInOut(duration: 0.1), value: engine.ledOn)

                Text("GRUVBOK")
                    .font(.title)
                    .fontWeight(.bold)
            }

            Spacer()

            // Audio status
            if engine.isAudioReady {
                HStack(spacing: 5) {
                    Image(systemName: "speaker.wave.2.fill")
                        .foregroundColor(.green)
                    Text("Audio Ready")
                        .font(.caption)
                        .foregroundColor(.green)
                }
            }
        }
    }

    private var knobSection: some View {
        HStack(spacing: 20) {
            KnobView(
                label: "Mode",
                value: Binding(
                    get: { Int(engine.rotaryPots[0]) },
                    set: { engine.setRotaryPot(0, value: UInt8($0)) }
                ),
                range: 0...127,
                displayText: "\(engine.currentMode)"
            )

            KnobView(
                label: "Tempo",
                value: Binding(
                    get: { Int(engine.rotaryPots[1]) },
                    set: { engine.setRotaryPot(1, value: UInt8($0)) }
                ),
                range: 0...127,
                displayText: "\(engine.tempo)"
            )

            KnobView(
                label: "Pattern",
                value: Binding(
                    get: { Int(engine.rotaryPots[2]) },
                    set: { engine.setRotaryPot(2, value: UInt8($0)) }
                ),
                range: 0...127,
                displayText: "\(engine.currentPattern + 1)"
            )

            KnobView(
                label: "Track",
                value: Binding(
                    get: { Int(engine.rotaryPots[3]) },
                    set: { engine.setRotaryPot(3, value: UInt8($0)) }
                ),
                range: 0...127,
                displayText: "\(engine.currentTrack + 1)"
            )
        }
    }

    private var sliderSection: some View {
        HStack(spacing: 15) {
            ForEach(0..<4) { slider in
                VStack {
                    Text("S\(slider + 1)")
                        .font(.caption)
                        .fontWeight(.semibold)

                    Slider(
                        value: Binding(
                            get: { Double(engine.sliderPots[slider]) },
                            set: { engine.setSliderPot(slider, value: UInt8($0)) }
                        ),
                        in: 0...127,
                        step: 1
                    )
                    #if os(iOS)
                    .rotationEffect(.degrees(-90))
                    .frame(width: 150, height: 40)
                    #else
                    .frame(height: 200)
                    #endif

                    Text("\(engine.sliderPots[slider])")
                        .font(.caption2)
                        .foregroundColor(.gray)
                }
            }
        }
    }

    private var patternGridSection: some View {
        VStack(alignment: .leading, spacing: 10) {
            Text("Pattern Grid - Track \(engine.currentTrack + 1)")
                .font(.headline)

            PatternGridView(
                events: engine.trackEvents,
                currentStep: engine.currentStep,
                onTap: { step in
                    engine.toggleButton(step)
                }
            )
        }
    }
}

#Preview("macOS") {
    ContentView(platform: "macOS")
        .frame(width: 900, height: 700)
}

#Preview("iOS") {
    ContentView(platform: "iOS")
}
