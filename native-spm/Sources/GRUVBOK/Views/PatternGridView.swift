import SwiftUI
import GRUVBOKBridge

/// 16-step pattern grid (4x4)
/// Shows active events, current step, and supports tap to toggle
struct PatternGridView: View {
    let events: [EventData]
    let currentStep: Int
    let onTap: (Int) -> Void

    private let columns = [
        GridItem(.flexible(), spacing: 3),
        GridItem(.flexible(), spacing: 3),
        GridItem(.flexible(), spacing: 3),
        GridItem(.flexible(), spacing: 3)
    ]

    var body: some View {
        LazyVGrid(columns: columns, spacing: 3) {
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
                // Shadow/depth layer
                RoundedRectangle(cornerRadius: 4)
                    .fill(Color.black.opacity(0.4))
                    .aspectRatio(1, contentMode: .fit)
                    .offset(y: 1)

                // Main button
                RoundedRectangle(cornerRadius: 4)
                    .fill(buttonGradient(for: step))
                    .aspectRatio(1, contentMode: .fit)
                    .overlay(
                        RoundedRectangle(cornerRadius: 4)
                            .stroke(buttonBorderColor(for: step), lineWidth: 1.5)
                    )
                    .shadow(
                        color: buttonShadowColor(for: step),
                        radius: step == currentStep ? 6 : 3
                    )

                // Event indicator dot (if has params)
                if step < events.count && events[step].switchOn {
                    VStack {
                        Spacer()
                        HStack {
                            Spacer()
                            Circle()
                                .fill(Color.cyan)
                                .frame(width: 4, height: 4)
                                .shadow(color: Color.cyan.opacity(0.8), radius: 2)
                                .padding(3)
                                .allowsHitTesting(false)
                        }
                    }
                    .allowsHitTesting(false)
                }
            }
            .contentShape(Rectangle())
        }
        .buttonStyle(.plain)
    }

    private func buttonGradient(for step: Int) -> LinearGradient {
        if step == currentStep {
            // Currently playing - subtle purple gradient
            return LinearGradient(
                colors: [Color.purple.opacity(0.5), Color.purple.opacity(0.3)],
                startPoint: .top,
                endPoint: .bottom
            )
        } else if step < events.count && events[step].switchOn {
            // Active event - cyan gradient
            return LinearGradient(
                colors: [Color.cyan.opacity(0.8), Color.cyan.opacity(0.5)],
                startPoint: .top,
                endPoint: .bottom
            )
        } else {
            // Empty - dark gradient
            return LinearGradient(
                colors: [Color(white: 0.25), Color(white: 0.15)],
                startPoint: .top,
                endPoint: .bottom
            )
        }
    }

    private func buttonBorderColor(for step: Int) -> Color {
        if step == currentStep {
            return Color.purple.opacity(0.6)
        } else if step < events.count && events[step].switchOn {
            return Color.cyan.opacity(0.7)
        } else {
            return Color.white.opacity(0.15)
        }
    }

    private func buttonShadowColor(for step: Int) -> Color {
        if step == currentStep {
            return Color.purple.opacity(0.4)
        } else if step < events.count && events[step].switchOn {
            return Color.cyan.opacity(0.4)
        } else {
            return Color.clear
        }
    }

}

// Preview disabled for SPM build
// #Preview {
//     VStack {
//         Text("Pattern Grid Demo")
//             .font(.headline)
//
//         PatternGridView(
//             events: [],
//             currentStep: 4,
//             onTap: { step in
//                 print("Tapped step \(step)")
//             }
//         )
//         .padding()
//     }
//     .frame(width: 400, height: 500)
//     .background(Color.black)
// }
