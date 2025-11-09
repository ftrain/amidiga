import SwiftUI

/// Rotary knob control with drag gesture
/// Works on both macOS and iOS
struct KnobView: View {
    let label: String
    @Binding var value: Int
    let range: ClosedRange<Int>
    var displayText: String?
    var size: CGFloat = 120

    @State private var angle: Double = 0
    @State private var isDragging: Bool = false
    @State private var lastDragValue: CGFloat = 0

    private let minAngle: Double = -135
    private let maxAngle: Double = 135

    var body: some View {
        VStack(spacing: 4) {
            ZStack {
                // Outer shadow/depth
                Circle()
                    .fill(
                        RadialGradient(
                            colors: [Color(white: 0.25), Color(white: 0.12)],
                            center: .center,
                            startRadius: 0,
                            endRadius: size / 2
                        )
                    )
                    .frame(width: size + 4, height: size + 4)
                    .shadow(color: Color.black.opacity(0.5), radius: 4, y: 2)

                // Main knob body with metallic gradient
                Circle()
                    .fill(
                        LinearGradient(
                            colors: [Color(white: 0.35), Color(white: 0.18), Color(white: 0.25)],
                            startPoint: .topLeading,
                            endPoint: .bottomTrailing
                        )
                    )
                    .frame(width: size, height: size)

                // Glossy highlight
                Circle()
                    .fill(
                        RadialGradient(
                            colors: [Color.white.opacity(0.15), Color.clear],
                            center: UnitPoint(x: 0.3, y: 0.3),
                            startRadius: 0,
                            endRadius: size / 2
                        )
                    )
                    .frame(width: size, height: size)

                // Value arc (progress indicator with glow)
                Circle()
                    .trim(from: 0, to: normalizedValue)
                    .stroke(
                        LinearGradient(
                            colors: [Color.cyan, Color.blue],
                            startPoint: .leading,
                            endPoint: .trailing
                        ),
                        style: StrokeStyle(lineWidth: 3, lineCap: .round)
                    )
                    .frame(width: size - 8, height: size - 8)
                    .rotationEffect(.degrees(-90))
                    .shadow(color: Color.cyan.opacity(0.6), radius: 3)

                // Indicator line with glow
                Rectangle()
                    .fill(isDragging ? Color.yellow : Color.cyan)
                    .frame(width: 2.5, height: size * 0.35)
                    .offset(y: -size * 0.25)
                    .rotationEffect(.degrees(angle))
                    .shadow(color: isDragging ? Color.yellow.opacity(0.8) : Color.cyan.opacity(0.8), radius: 4)

                // Center cap
                Circle()
                    .fill(
                        RadialGradient(
                            colors: [Color(white: 0.3), Color(white: 0.15)],
                            center: .center,
                            startRadius: 0,
                            endRadius: size * 0.1
                        )
                    )
                    .frame(width: size * 0.2, height: size * 0.2)
                    .overlay(
                        Circle()
                            .stroke(Color.black.opacity(0.3), lineWidth: 1)
                    )
            }
            .gesture(dragGesture)
            .onAppear {
                updateAngle()
            }
            .onChange(of: value) {
                updateAngle()
            }

            // Label
            Text(label)
                .font(.system(size: 12, weight: .semibold, design: .rounded))
                .foregroundColor(.cyan)
                .textCase(.uppercase)

            // Display value
            Text(displayText ?? "\(value)")
                .font(.system(size: 12, weight: .medium, design: .monospaced))
                .foregroundColor(.white.opacity(0.8))
                .padding(.horizontal, 6)
                .padding(.vertical, 2)
                .background(
                    RoundedRectangle(cornerRadius: 3)
                        .fill(Color(white: 0.15))
                        .overlay(
                            RoundedRectangle(cornerRadius: 3)
                                .stroke(Color.cyan.opacity(0.3), lineWidth: 1)
                        )
                )
        }
        .frame(width: size + 10)
    }

    private var normalizedValue: Double {
        let t = Double(value - range.lowerBound) / Double(range.upperBound - range.lowerBound)
        return max(0, min(1, t))
    }

    private func updateAngle() {
        angle = minAngle + (maxAngle - minAngle) * normalizedValue
    }

    private var dragGesture: some Gesture {
        DragGesture(minimumDistance: 0)
            .onChanged { gesture in
                if !isDragging {
                    isDragging = true
                    lastDragValue = gesture.translation.height
                }

                // Calculate delta since last update
                let currentDragValue = gesture.translation.height
                let delta = -(currentDragValue - lastDragValue)
                lastDragValue = currentDragValue

                let sensitivity: Double = 0.3  // Reduced sensitivity

                // Calculate new value
                let change = Int(delta * sensitivity)
                if change != 0 {
                    let newValue = max(range.lowerBound, min(range.upperBound, value + change))
                    if newValue != value {
                        value = newValue

                        // Haptic feedback on value change (iOS only)
                        #if os(iOS)
                        let generator = UIImpactFeedbackGenerator(style: .light)
                        generator.impactOccurred()
                        #endif
                    }
                }
            }
            .onEnded { _ in
                isDragging = false
                lastDragValue = 0
            }
    }
}

// Preview disabled for SPM build
// #Preview {
//     VStack(spacing: 30) {
//         HStack(spacing: 30) {
//             KnobView(
//                 label: "Mode",
//                 value: .constant(5),
//                 range: 0...14,
//                 displayText: "5"
//             )
//
//             KnobView(
//                 label: "Tempo",
//                 value: .constant(120),
//                 range: 60...240,
//                 displayText: "120 BPM"
//             )
//         }
//     }
//     .padding()
//     .background(Color.black)
// }
