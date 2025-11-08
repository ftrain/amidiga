import SwiftUI
import GRUVBOKBridge

struct SongDataExplorerView: View {
    @ObservedObject var engine: EngineState
    @State private var editingValues: [Int: [Int: String]] = [:] // [step: [pot: value]]

    var body: some View {
        VStack(alignment: .leading, spacing: 15) {
            // Navigation controls - 4 knobs matching hardware tab
            HStack(spacing: 15) {
                // Mode knob with mode name
                VStack(spacing: 0) {
                    KnobView(
                        label: "Mode",
                        value: Binding(
                            get: { Int(engine.rotaryPots[0]) },
                            set: { engine.setRotaryPot(0, value: UInt8($0)) }
                        ),
                        range: 0...127,
                        displayText: "\(engine.currentMode)",
                        size: 60
                    )
                    Text(ModeLabels.getModeName(engine.currentMode))
                        .font(.system(size: 10))
                        .foregroundColor(.secondary)
                        .lineLimit(1)
                        .frame(height: 14)
                }
                .frame(width: 70, alignment: .top)

                // Tempo knob with spacer to match Mode knob height
                VStack(spacing: 0) {
                    KnobView(
                        label: "Tempo",
                        value: Binding(
                            get: { Int(engine.rotaryPots[1]) },
                            set: { engine.setRotaryPot(1, value: UInt8($0)) }
                        ),
                        range: 0...127,
                        displayText: "\(engine.tempo) BPM",
                        size: 60
                    )
                    Spacer()
                        .frame(height: 14)
                }
                .frame(width: 70, alignment: .top)

                // Pattern knob with spacer to match Mode knob height
                VStack(spacing: 0) {
                    KnobView(
                        label: "Pattern",
                        value: Binding(
                            get: { Int(engine.rotaryPots[2]) },
                            set: { engine.setRotaryPot(2, value: UInt8($0)) }
                        ),
                        range: 0...127,
                        displayText: "\(engine.currentPattern + 1)",
                        size: 60
                    )
                    Spacer()
                        .frame(height: 14)
                }
                .frame(width: 70, alignment: .top)

                // Track knob with spacer to match Mode knob height
                VStack(spacing: 0) {
                    KnobView(
                        label: "Track",
                        value: Binding(
                            get: { Int(engine.rotaryPots[3]) },
                            set: { engine.setRotaryPot(3, value: UInt8($0)) }
                        ),
                        range: 0...127,
                        displayText: "\(engine.currentTrack + 1)",
                        size: 60
                    )
                    Spacer()
                        .frame(height: 14)
                }
                .frame(width: 70, alignment: .top)

                Spacer()

                // Capacity info
                VStack(alignment: .trailing, spacing: 2) {
                    Text("Capacity")
                        .font(.system(size: 12, weight: .semibold))
                        .foregroundColor(.cyan)
                    Text("61,440 events")
                        .font(.system(size: 11))
                        .foregroundColor(.gray)
                    Text("~245KB")
                        .font(.system(size: 11))
                        .foregroundColor(.gray)
                }
            }
            .padding(.horizontal)
            .padding(.vertical)

            Divider()

            // Event table
            eventTable
        }
    }

    private var eventTable: some View {
        VStack(alignment: .leading, spacing: 0) {
            // Header
            HStack(spacing: 0) {
                Text("Step")
                    .frame(width: 60, alignment: .center)
                    .padding(.vertical, 10)
                    .background(Color.secondary.opacity(0.2))

                Text("Switch")
                    .frame(width: 80, alignment: .center)
                    .padding(.vertical, 10)
                    .background(Color.secondary.opacity(0.2))

                ForEach(0..<4) { pot in
                    Text("S\(pot + 1)")
                        .frame(maxWidth: .infinity, alignment: .center)
                        .padding(.vertical, 10)
                        .background(Color.secondary.opacity(0.2))
                }
            }
            .font(.system(size: 13, weight: .bold))
            .foregroundColor(.cyan)

            Divider()

            // Rows
            ScrollView {
                VStack(spacing: 0) {
                    ForEach(Array(engine.trackEvents.enumerated()), id: \.element.step) { index, event in
                        HStack(spacing: 0) {
                            // Step number
                            Text("\(index + 1)")
                                .frame(width: 60, alignment: .center)
                                .padding(.vertical, 8)
                                .background(index == (engine.currentMode == 0 ? engine.songModeStep : engine.currentStep) ? Color.cyan.opacity(0.3) : Color.clear)

                            // Switch (editable with tap)
                            Image(systemName: event.switchOn ? "checkmark.circle.fill" : "circle")
                                .foregroundColor(event.switchOn ? .cyan : .gray)
                                .frame(width: 80, alignment: .center)
                                .padding(.vertical, 8)
                                .background(index == (engine.currentMode == 0 ? engine.songModeStep : engine.currentStep) ? Color.cyan.opacity(0.3) : Color.clear)
                                .onTapGesture {
                                    engine.toggleButton(index)
                                }

                            // Pot values (editable)
                            ForEach(0..<4) { pot in
                                TextField("", text: Binding(
                                    get: {
                                        editingValues[index]?[pot] ?? "\(event.pots[pot].intValue)"
                                    },
                                    set: { newValue in
                                        if editingValues[index] == nil {
                                            editingValues[index] = [:]
                                        }
                                        editingValues[index]?[pot] = newValue

                                        // Auto-save: validate and apply
                                        if let intValue = Int(newValue), intValue >= 0, intValue <= 127 {
                                            let clampedValue = UInt8(min(127, max(0, intValue)))
                                            // Set slider value
                                            engine.setSliderPot(pot, value: clampedValue)
                                            // Toggle button to save
                                            engine.toggleButton(index)
                                        }
                                    }
                                ))
                                .textFieldStyle(.plain)
                                .multilineTextAlignment(.center)
                                .frame(maxWidth: .infinity, alignment: .center)
                                .padding(.vertical, 8)
                                .background(index == (engine.currentMode == 0 ? engine.songModeStep : engine.currentStep) ? Color.cyan.opacity(0.3) : Color.clear)
                                .foregroundColor(event.switchOn ? .primary : .secondary)
                            }
                        }
                        .font(.system(size: 12, design: .monospaced))

                        if index < 15 {
                            Divider()
                        }
                    }
                }
            }
        }
        .overlay(
            RoundedRectangle(cornerRadius: 4)
                .stroke(Color.secondary.opacity(0.3), lineWidth: 1)
        )
        .padding(.horizontal)
    }
}
