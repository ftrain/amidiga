import SwiftUI
import UniformTypeIdentifiers

struct HardwareControlsView: View {
    @ObservedObject var engine: EngineState
    @State private var editingCell: (step: Int, pot: Int)? = nil

    var body: some View {
        GeometryReader { geometry in
            let totalWidth = geometry.size.width
            let totalHeight = geometry.size.height
            let cellWidth = (totalWidth - 36) / 2
            let cellHeight = (totalHeight - 36) / 2

            VStack(spacing: 12) {
                // Top row
                HStack(spacing: 12) {
                    slidersSection(width: cellWidth, height: cellHeight)
                    patternGridSection(width: cellWidth, height: cellHeight)
                }

                // Bottom row
                HStack(spacing: 12) {
                    knobsSection(width: cellWidth, height: cellHeight)
                    eventDataSection(width: cellWidth, height: cellHeight)
                }
            }
            .padding(16)
            .background(Color.black)
        }
    }

    // MARK: - Top Left: Sliders

    private func slidersSection(width: CGFloat, height: CGFloat) -> some View {
        let availableHeight = height - 40  // Less header space
        let sliderHeight = max(120, availableHeight - 60)
        let sliderWidth = max(40, (width - 60) / 4)

        return VStack(spacing: 0) {
            HStack(spacing: max(8, (width - (sliderWidth * 4) - 40) / 3)) {
                ForEach(0..<4) { slider in
                    VStack(spacing: 6) {
                        // Value display
                        Text("\(engine.sliderPots[slider])")
                            .font(.system(size: 13, weight: .bold, design: .monospaced))
                            .foregroundColor(.cyan)
                            .monospacedDigit()
                            .frame(width: sliderWidth, height: 22)
                            .background(
                                RoundedRectangle(cornerRadius: 3)
                                    .fill(Color(white: 0.08))
                                    .overlay(
                                        RoundedRectangle(cornerRadius: 3)
                                            .stroke(Color.cyan.opacity(0.4), lineWidth: 1)
                                    )
                            )

                        // Slider track
                        ZStack {
                            RoundedRectangle(cornerRadius: 3)
                                .fill(Color(white: 0.08))
                                .frame(width: 10, height: sliderHeight)

                            let normalizedValue = Double(engine.sliderPots[slider]) / 127.0
                            VStack {
                                Spacer()
                                RoundedRectangle(cornerRadius: 3)
                                    .fill(LinearGradient(
                                        colors: [Color.blue.opacity(0.6), Color.cyan.opacity(0.8)],
                                        startPoint: .top,
                                        endPoint: .bottom
                                    ))
                                    .frame(width: 10, height: sliderHeight * normalizedValue)
                            }

                            Slider(
                                value: Binding(
                                    get: { Double(engine.sliderPots[slider]) },
                                    set: { engine.setSliderPot(slider, value: UInt8($0)) }
                                ),
                                in: 0...127,
                                step: 1
                            )
                            .rotationEffect(.degrees(-90))
                            .frame(width: sliderHeight)
                            .accentColor(.clear)
                        }
                        .frame(width: sliderWidth, height: sliderHeight)

                        // Labels
                        VStack(spacing: 2) {
                            Text(ModeLabels.getSliderLabel(sliderIndex: slider, modeNumber: engine.currentMode))
                                .font(.system(size: 13, weight: .semibold, design: .rounded))
                                .foregroundColor(.cyan.opacity(0.9))
                                .lineLimit(1)
                                .frame(width: sliderWidth + 10)

                            Text("S\(slider + 1)")
                                .font(.system(size: 13, weight: .medium, design: .monospaced))
                                .foregroundColor(.gray)
                        }
                        .frame(height: 28)
                    }
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .center)
        }
        .frame(width: width, height: height)
        .padding(10)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(Color(white: 0.1))
                .overlay(
                    RoundedRectangle(cornerRadius: 8)
                        .stroke(Color.cyan.opacity(0.3), lineWidth: 1)
                )
        )
    }

    // MARK: - Top Right: Pattern Grid

    private func patternGridSection(width: CGFloat, height: CGFloat) -> some View {
        let availableSize = min(width - 20, height - 20)
        let gridSize = max(220, availableSize)

        return VStack(spacing: 0) {
            PatternGridView(
                events: engine.trackEvents,
                currentStep: engine.currentMode == 0 ? engine.songModeStep : engine.currentStep,
                onTap: { step in
                    engine.toggleButton(step)
                }
            )
            .frame(width: gridSize, height: gridSize)
            .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
        .frame(width: width, height: height)
        .padding(10)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(Color(white: 0.1))
                .overlay(
                    RoundedRectangle(cornerRadius: 8)
                        .stroke(Color.cyan.opacity(0.3), lineWidth: 1)
                )
        )
    }

    // MARK: - Bottom Left: Knobs

    private func knobsSection(width: CGFloat, height: CGFloat) -> some View {
        let availableSize = min(width - 40, height - 40)
        let knobSize = max(65, min(110, availableSize / 2.5))
        let spacing = max(12, (width - (knobSize * 2) - 40) / 3)

        return VStack(spacing: 0) {
            VStack(spacing: spacing) {
                HStack(spacing: spacing) {
                    // Mode knob
                    VStack(spacing: 4) {
                        KnobView(
                            label: "Mode",
                            value: Binding(
                                get: { Int(engine.rotaryPots[0]) },
                                set: { engine.setRotaryPot(0, value: UInt8($0)) }
                            ),
                            range: 0...127,
                            displayText: "\(engine.currentMode)",
                            size: knobSize
                        )
                        Text(ModeLabels.getModeName(engine.currentMode))
                            .font(.system(size: 10))
                            .foregroundColor(.secondary)
                            .lineLimit(1)
                            .frame(height: 14)
                    }

                    // Tempo knob
                    VStack(spacing: 4) {
                        KnobView(
                            label: "Tempo",
                            value: Binding(
                                get: { Int(engine.rotaryPots[1]) },
                                set: { engine.setRotaryPot(1, value: UInt8($0)) }
                            ),
                            range: 0...127,
                            displayText: "\(engine.tempo) BPM",
                            size: knobSize
                        )
                        Spacer().frame(height: 14)
                    }
                }

                HStack(spacing: spacing) {
                    // Pattern knob
                    KnobView(
                        label: "Pattern",
                        value: Binding(
                            get: { Int(engine.rotaryPots[2]) },
                            set: { engine.setRotaryPot(2, value: UInt8($0)) }
                        ),
                        range: 0...127,
                        displayText: "\(engine.currentPattern + 1)",
                        size: knobSize
                    )

                    // Track knob
                    KnobView(
                        label: "Track",
                        value: Binding(
                            get: { Int(engine.rotaryPots[3]) },
                            set: { engine.setRotaryPot(3, value: UInt8($0)) }
                        ),
                        range: 0...127,
                        displayText: "\(engine.currentTrack + 1)",
                        size: knobSize
                    )
                }
            }
            .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
        .frame(width: width, height: height)
        .padding(10)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(Color(white: 0.1))
                .overlay(
                    RoundedRectangle(cornerRadius: 8)
                        .stroke(Color.cyan.opacity(0.3), lineWidth: 1)
                )
        )
    }

    // MARK: - Bottom Right: Event Data

    private func eventDataSection(width: CGFloat, height: CGFloat) -> some View {
        let rowHeight = (height - 60) / 17  // 1 header + 16 rows

        return VStack(spacing: 0) {
            // Header row
            HStack(spacing: 6) {
                Text("#")
                    .font(.system(size: 13, weight: .bold, design: .monospaced))
                    .foregroundColor(.cyan)
                    .frame(width: 24, alignment: .trailing)

                Text("•")
                    .font(.system(size: 13, weight: .bold))
                    .foregroundColor(.cyan)
                    .frame(width: 28)

                ForEach(0..<4) { pot in
                    Text("S\(pot + 1)")
                        .font(.system(size: 13, weight: .bold, design: .monospaced))
                        .foregroundColor(.cyan)
                        .frame(maxWidth: .infinity)
                }
            }
            .padding(.vertical, 4)
            .padding(.horizontal, 8)
            .background(Color(white: 0.08))

            // Event rows
            VStack(spacing: 0) {
                ForEach(Array(engine.trackEvents.enumerated()), id: \.element.step) { index, event in
                    HStack(spacing: 6) {
                        // Step number
                        Text("\(index + 1)")
                            .font(.system(size: 13, design: .monospaced))
                            .foregroundColor(.white)
                            .frame(width: 24, alignment: .trailing)

                        // Switch button
                        Button(action: {
                            engine.toggleButton(index)
                        }) {
                            Image(systemName: event.switchOn ? "checkmark.circle.fill" : "circle")
                                .foregroundColor(event.switchOn ? .cyan : .gray)
                                .font(.system(size: 13))
                                .frame(width: 28)
                        }
                        .buttonStyle(.plain)

                        // Pot values (virtual slider - drag to change)
                        ForEach(0..<4) { pot in
                            Text("\(event.pots[pot].intValue)")
                                .font(.system(size: 13, weight: .medium, design: .monospaced))
                                .foregroundColor(
                                    editingCell?.step == index && editingCell?.pot == pot
                                    ? .cyan
                                    : (event.switchOn ? .white : .secondary)
                                )
                                .frame(maxWidth: .infinity)
                                .frame(height: rowHeight - 2)
                                .background(
                                    editingCell?.step == index && editingCell?.pot == pot
                                    ? Color.cyan.opacity(0.2)
                                    : Color.clear
                                )
                                .cornerRadius(2)
                                .gesture(
                                    DragGesture(minimumDistance: 0)
                                        .onChanged { drag in
                                            // Start editing on click
                                            if editingCell?.step != index || editingCell?.pot != pot {
                                                editingCell = (step: index, pot: pot)
                                            }

                                            // Calculate new value based on horizontal drag
                                            let sensitivity: CGFloat = 0.5
                                            let delta = Int(drag.translation.width * sensitivity)
                                            let currentValue = Int(event.pots[pot].intValue)
                                            let newValue = max(0, min(127, currentValue + delta))

                                            if newValue != currentValue {
                                                engine.setSliderPot(pot, value: UInt8(newValue))
                                                engine.toggleButton(index)
                                            }
                                        }
                                        .onEnded { _ in
                                            // Stop editing when drag ends
                                            editingCell = nil
                                        }
                                )
                        }
                    }
                    .frame(height: rowHeight)
                    .padding(.horizontal, 8)
                    .background(
                        index == (engine.currentMode == 0 ? engine.songModeStep : engine.currentStep) ?
                        Color.cyan.opacity(0.15) : Color.clear
                    )
                }
            }
        }
        .frame(width: width, height: height)
        .padding(10)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(Color(white: 0.1))
                .overlay(
                    RoundedRectangle(cornerRadius: 8)
                        .stroke(Color.cyan.opacity(0.3), lineWidth: 1)
                )
        )
    }
}
