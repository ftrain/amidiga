import SwiftUI

/// 16-step pattern grid (4x4)
/// Shows active events, current step, and supports tap to toggle
struct PatternGridView: View {
    let events: [EventData]
    let currentStep: Int
    let onTap: (Int) -> Void

    private let columns = [
        GridItem(.flexible(), spacing: 4),
        GridItem(.flexible(), spacing: 4),
        GridItem(.flexible(), spacing: 4),
        GridItem(.flexible(), spacing: 4)
    ]

    var body: some View {
        LazyVGrid(columns: columns, spacing: 4) {
            ForEach(0..<16, id: \.self) { step in
                stepButton(for: step)
            }
        }
    }

    private func stepButton(for step: Int) -> some View {
        Button(action: {
            onTap(step)

            // Haptic feedback on tap (iOS only)
            #if os(iOS)
            let generator = UIImpactFeedbackGenerator(style: .medium)
            generator.impactOccurred()
            #endif
        }) {
            ZStack {
                RoundedRectangle(cornerRadius: 8)
                    .fill(buttonColor(for: step))
                    .aspectRatio(1, contentMode: .fit)

                // Step number
                Text("\(step + 1)")
                    .font(.headline)
                    .fontWeight(.bold)
                    .foregroundColor(.white)

                // Event indicator dot (if has params)
                if step < events.count && events[step].switchOn {
                    VStack {
                        Spacer()
                        HStack {
                            Spacer()
                            Circle()
                                .fill(Color.white.opacity(0.3))
                                .frame(width: 6, height: 6)
                                .padding(4)
                        }
                    }
                }
            }
        }
        .buttonStyle(.plain)
    }

    private func buttonColor(for step: Int) -> Color {
        if step == currentStep {
            // Currently playing - red
            return Color.red
        } else if step < events.count && events[step].switchOn {
            // Active event - green
            return Color.green.opacity(0.8)
        } else {
            // Empty - gray
            return Color.gray.opacity(0.4)
        }
    }
}

#Preview {
    VStack {
        Text("Pattern Grid Demo")
            .font(.headline)

        PatternGridView(
            events: [
                EventData(step: 0, switchOn: true, pots: [64, 64, 64, 64]),
                EventData(step: 1, switchOn: false, pots: [0, 0, 0, 0]),
                EventData(step: 4, switchOn: true, pots: [80, 60, 40, 100]),
                EventData(step: 8, switchOn: true, pots: [100, 100, 100, 100]),
            ] + (0..<12).map { EventData(step: $0, switchOn: false, pots: [0, 0, 0, 0]) },
            currentStep: 4,
            onTap: { step in
                print("Tapped step \(step)")
            }
        )
        .padding()
    }
    .frame(width: 400, height: 500)
    .background(Color.black)
}
