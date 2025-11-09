import Foundation
import GRUVBOKBridge

/// Provides mode-specific labels for slider pots
struct ModeLabels {
    static func getSliderLabel(sliderIndex: Int, modeNumber: Int, engine: EngineState) -> String {
        let labels = engine.getSliderLabels(modeNumber)

        guard sliderIndex >= 0 && sliderIndex < labels.count else {
            return "S\(sliderIndex + 1)"
        }

        return labels[sliderIndex]
    }

    static func getModeName(_ modeNumber: Int, engine: EngineState) -> String {
        return engine.getModeName(modeNumber)
    }
}
