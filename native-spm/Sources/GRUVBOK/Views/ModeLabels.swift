import Foundation

/// Provides mode-specific labels for slider pots
struct ModeLabels {
    static func getSliderLabel(sliderIndex: Int, modeNumber: Int) -> String {
        let labels: [String]

        switch modeNumber {
        case 0:  // Song/Pattern Sequencer
            labels = ["Pattern", "---", "---", "---"]
        case 1:  // Drums
            labels = ["Velocity", "Length", "S3", "S4"]
        case 2:  // Acid
            labels = ["Pitch", "Length", "Slide", "Filter"]
        case 3:  // Chords
            labels = ["Root", "Type", "Velocity", "Length"]
        case 4:  // Arpeggiator
            labels = ["Root", "Pattern", "Velocity", "Length"]
        case 5:  // Euclidean
            labels = ["Hits", "Rotate", "Pitch", "Velocity"]
        case 6:  // Random
            labels = ["Prob", "Center", "Range", "Velocity"]
        case 7:  // Sample & Hold
            labels = ["Rate", "Quant", "Glitch", "Mod"]
        default:
            labels = ["S1", "S2", "S3", "S4"]
        }

        guard sliderIndex >= 0 && sliderIndex < labels.count else {
            return "S\(sliderIndex + 1)"
        }

        return labels[sliderIndex]
    }

    static func getModeName(_ modeNumber: Int) -> String {
        switch modeNumber {
        case 0: return "Song/Pattern"
        case 1: return "Drums"
        case 2: return "Acid"
        case 3: return "Chords"
        case 4: return "Arpeggiator"
        case 5: return "Euclidean"
        case 6: return "Random"
        case 7: return "Sample & Hold"
        default: return "Mode \(modeNumber)"
        }
    }
}
