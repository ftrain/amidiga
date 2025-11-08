import SwiftUI

/// Rotary knob control with drag gesture
/// Works on both macOS and iOS
struct KnobView: View {
    let label: String
    @Binding var value: Int
    let range: ClosedRange<Int>
    var displayText: String?
    var size: CGFloat = 70

    @State private var angle: Double = 0
    @State private var isDragging: Bool = false

    private let minAngle: Double = -135
    private let maxAngle: Double = 135

    var body: some View {
        VStack(spacing: 8) {
            ZStack {
                // Background circle
                Circle()
                    .fill(Color(white: 0.2))
                    .frame(width: size, height: size)

                // Outer ring
                Circle()
                    .stroke(
                        LinearGradient(
                            colors: [.blue, .purple],
                            startPoint: .topLeading,
                            endPoint: .bottomTrailing
                        ),
                        lineWidth: 3
                    )
                    .frame(width: size, height: size)

                // Value arc (progress indicator)
                Circle()
                    .trim(from: 0, to: normalizedValue)
                    .stroke(Color.blue, lineWidth: 4)
                    .frame(width: size - 8, height: size - 8)
                    .rotationEffect(.degrees(-90))

                // Indicator line
                Rectangle()
                    .fill(isDragging ? Color.yellow : Color.white)
                    .frame(width: 3, height: size * 0.3)
                    .offset(y: -size * 0.25)
                    .rotationEffect(.degrees(angle))

                // Center dot
                Circle()
                    .fill(Color.gray)
                    .frame(width: size * 0.15, height: size * 0.15)
            }
            .gesture(dragGesture)
            .onAppear {
                updateAngle()
            }
            .onChange(of: value) { _ in
                updateAngle()
            }

            // Label
            Text(label)
                .font(.caption)
                .fontWeight(.medium)

            // Display value
            Text(displayText ?? "\(value)")
                .font(.caption2)
                .foregroundColor(.gray)
                .monospacedDigit()
        }
        .frame(width: size + 20)
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
                isDragging = true

                // Calculate delta based on vertical drag
                let delta = -gesture.translation.height
                let sensitivity: Double = 0.5  // Adjust for feel

                // Calculate new value
                let valueRange = Double(range.upperBound - range.lowerBound)
                let change = Int(delta * sensitivity)
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
            .onEnded { _ in
                isDragging = false
            }
    }
}

#Preview {
    VStack(spacing: 30) {
        HStack(spacing: 30) {
            KnobView(
                label: "Mode",
                value: .constant(5),
                range: 0...14,
                displayText: "5"
            )

            KnobView(
                label: "Tempo",
                value: .constant(120),
                range: 60...240,
                displayText: "120 BPM"
            )
        }
    }
    .padding()
    .background(Color.black)
}
