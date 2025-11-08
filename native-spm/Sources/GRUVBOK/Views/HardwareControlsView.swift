import SwiftUI
import UniformTypeIdentifiers
import GRUVBOKBridge

struct HardwareControlsView: View {
    @ObservedObject var engine: EngineState
    @State private var editingCell: (step: Int, pot: Int)? = nil

    var body: some View {
        GeometryReader { geometry in
            let spacing: CGFloat = 4
            let cellWidth = (geometry.size.width - spacing) / 2
            let cellHeight = (geometry.size.height - spacing) / 2

            VStack(spacing: spacing) {
                HStack(spacing: spacing) {
                    slidersSection(width: cellWidth, height: cellHeight)
                    patternGridSection(width: cellWidth, height: cellHeight)
                }
                HStack(spacing: spacing) {
                    knobsSection(width: cellWidth, height: cellHeight)
                    eventDataSection(width: cellWidth, height: cellHeight)
                }
            }
            .background(Color.black)
        }
    }

    // MARK: - Sliders

    private func slidersSection(width: CGFloat, height: CGFloat) -> some View {
        GeometryReader { geo in
            let availableHeight = geo.size.height - 80  // Reserve space for labels and value
            let sliderHeight = max(100, availableHeight)
            let sliderWidth = (geo.size.width - 16) / 4

            HStack(spacing: 1) {
                ForEach(0..<4) { slider in
                    VStack(spacing: 2) {
                        // Value
                        Text("\(engine.sliderPots[slider])")
                            .font(.system(size: 12, weight: .bold, design: .monospaced))
                            .foregroundColor(.cyan)
                            .frame(height: 20)

                        // Slider
                        ZStack {
                            RoundedRectangle(cornerRadius: 2)
                                .fill(Color(white: 0.08))
                                .frame(width: 8)

                            VStack {
                                Spacer()
                                RoundedRectangle(cornerRadius: 2)
                                    .fill(LinearGradient(
                                        colors: [Color.blue.opacity(0.6), Color.cyan.opacity(0.8)],
                                        startPoint: .top,
                                        endPoint: .bottom
                                    ))
                                    .frame(width: 8, height: sliderHeight * CGFloat(engine.sliderPots[slider]) / 127.0)
                            }

                            // Invisible drag area
                            Rectangle()
                                .fill(Color.clear)
                                .contentShape(Rectangle())
                                .gesture(
                                    DragGesture(minimumDistance: 0)
                                        .onChanged { drag in
                                            let progress = 1.0 - (drag.location.y / sliderHeight)
                                            let clampedProgress = max(0, min(1, progress))
                                            let newValue = UInt8(clampedProgress * 127.0)
                                            engine.setSliderPot(slider, value: newValue)
                                        }
                                )
                        }
                        .frame(height: sliderHeight)

                        // Labels
                        Text(ModeLabels.getSliderLabel(sliderIndex: slider, modeNumber: engine.currentMode))
                            .font(.system(size: 12, weight: .medium))
                            .foregroundColor(.cyan.opacity(0.8))
                            .lineLimit(1)
                            .minimumScaleFactor(0.5)
                            .frame(width: sliderWidth, height: 16)

                        Text("S\(slider + 1)")
                            .font(.system(size: 12, weight: .medium, design: .monospaced))
                            .foregroundColor(.gray)
                            .frame(height: 16)
                    }
                    .frame(width: sliderWidth)
                }
            }
            .padding(4)
        }
        .frame(width: width, height: height)
        .background(
            RoundedRectangle(cornerRadius: 4)
                .fill(Color(white: 0.1))
                .overlay(RoundedRectangle(cornerRadius: 4).stroke(Color.cyan.opacity(0.2), lineWidth: 1))
        )
    }

    // MARK: - Pattern Grid

    private func patternGridSection(width: CGFloat, height: CGFloat) -> some View {
        GeometryReader { geo in
            let size = min(geo.size.width, geo.size.height) - 8

            PatternGridView(
                events: engine.trackEvents,
                currentStep: engine.currentMode == 0 ? engine.songModeStep : engine.currentStep,
                onTap: { step in engine.toggleButton(step) }
            )
            .frame(width: size, height: size)
            .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
        .frame(width: width, height: height)
        .background(
            RoundedRectangle(cornerRadius: 4)
                .fill(Color(white: 0.1))
                .overlay(RoundedRectangle(cornerRadius: 4).stroke(Color.cyan.opacity(0.2), lineWidth: 1))
        )
    }

    // MARK: - Knobs

    private func knobsSection(width: CGFloat, height: CGFloat) -> some View {
        GeometryReader { geo in
            let knobSize = min(geo.size.width, geo.size.height) / 3.2
            let spacing: CGFloat = 8

            VStack(spacing: spacing) {
                HStack(spacing: spacing) {
                    // Mode
                    VStack(spacing: 2) {
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
                            .font(.system(size: 12))
                            .foregroundColor(.secondary)
                            .lineLimit(1)
                            .minimumScaleFactor(0.5)
                    }
                    .frame(maxWidth: .infinity, maxHeight: .infinity)

                    // Tempo
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
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                }

                HStack(spacing: spacing) {
                    // Pattern
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
                    .frame(maxWidth: .infinity, maxHeight: .infinity)

                    // Track
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
                    .frame(maxWidth: .infinity, maxHeight: .infinity)
                }
            }
            .padding(4)
        }
        .frame(width: width, height: height)
        .background(
            RoundedRectangle(cornerRadius: 4)
                .fill(Color(white: 0.1))
                .overlay(RoundedRectangle(cornerRadius: 4).stroke(Color.cyan.opacity(0.2), lineWidth: 1))
        )
    }

    // MARK: - Event Data

    private func eventDataSection(width: CGFloat, height: CGFloat) -> some View {
        let verticalPadding = max(1, min(3, height / 150))  // Responsive: 1-3px based on height

        return VStack(spacing: 0) {
            eventHeaderRow(fontSize: 12)
            eventListView(fontSize: 12, verticalPadding: verticalPadding)
        }
        .padding(4)
        .frame(width: width, height: height)
        .background(
            RoundedRectangle(cornerRadius: 4)
                .fill(Color(white: 0.1))
                .overlay(RoundedRectangle(cornerRadius: 4).stroke(Color.cyan.opacity(0.2), lineWidth: 1))
        )
    }

    private func eventHeaderRow(fontSize: CGFloat) -> some View {
        HStack(spacing: 4) {
            Text("#")
                .font(.system(size: fontSize, weight: .bold, design: .monospaced))
                .foregroundColor(.cyan)
                .frame(width: 20, alignment: .trailing)

            Text("•")
                .font(.system(size: fontSize, weight: .bold))
                .foregroundColor(.cyan)
                .frame(width: 20)

            ForEach(0..<4) { pot in
                Text("S\(pot + 1)")
                    .font(.system(size: fontSize, weight: .bold, design: .monospaced))
                    .foregroundColor(.cyan)
                    .frame(maxWidth: .infinity)
            }
        }
        .padding(.vertical, 2)
        .padding(.horizontal, 4)
        .background(Color(white: 0.08))
    }

    private func eventListView(fontSize: CGFloat, verticalPadding: CGFloat) -> some View {
        ScrollView {
            VStack(spacing: 0) {
                ForEach(Array(engine.trackEvents.enumerated()), id: \.element.step) { index, event in
                    eventRow(index: index, event: event, fontSize: fontSize, verticalPadding: verticalPadding)
                }
            }
        }
        .frame(maxHeight: .infinity)
    }

    private func eventRow(index: Int, event: EventData, fontSize: CGFloat, verticalPadding: CGFloat) -> some View {
        HStack(spacing: 4) {
            Text("\(index + 1)")
                .font(.system(size: fontSize, design: .monospaced))
                .foregroundColor(.white)
                .frame(width: 20, alignment: .trailing)

            Button(action: { engine.toggleButton(index) }) {
                Image(systemName: event.switchOn ? "checkmark.circle.fill" : "circle")
                    .foregroundColor(event.switchOn ? .cyan : .gray)
                    .font(.system(size: fontSize))
                    .frame(width: 20)
            }
            .buttonStyle(.plain)

            ForEach(0..<4) { pot in
                eventPotCell(index: index, pot: pot, event: event, fontSize: fontSize)
            }
        }
        .padding(.horizontal, 4)
        .padding(.vertical, verticalPadding)
        .background(
            index == (engine.currentMode == 0 ? engine.songModeStep : engine.currentStep)
            ? Color.cyan.opacity(0.15) : Color.clear
        )
    }

    private func eventPotCell(index: Int, pot: Int, event: EventData, fontSize: CGFloat) -> some View {
        Text("\(event.pots[pot].intValue)")
            .font(.system(size: fontSize, weight: .medium, design: .monospaced))
            .foregroundColor(
                editingCell?.step == index && editingCell?.pot == pot
                ? .cyan : (event.switchOn ? .white : .secondary)
            )
            .frame(maxWidth: .infinity, minHeight: 20)
            .background(
                editingCell?.step == index && editingCell?.pot == pot
                ? Color.cyan.opacity(0.2) : Color.clear
            )
            .cornerRadius(2)
            .gesture(
                DragGesture(minimumDistance: 0)
                    .onChanged { drag in
                        if editingCell?.step != index || editingCell?.pot != pot {
                            editingCell = (step: index, pot: pot)
                        }
                        let delta = Int(drag.translation.width * 0.5)
                        let currentValue = Int(event.pots[pot].intValue)
                        let newValue = max(0, min(127, currentValue + delta))
                        if newValue != currentValue {
                            engine.setSliderPot(pot, value: UInt8(newValue))
                            engine.toggleButton(index)
                        }
                    }
                    .onEnded { _ in editingCell = nil }
            )
    }
}
