import SwiftUI

@main
struct GruvbokApp: App {
    init() {
        // Keep screen awake during music programming
        UIApplication.shared.isIdleTimerDisabled = true
    }

    var body: some Scene {
        WindowGroup {
            ContentView(platform: "iOS")
                .preferredColorScheme(.dark)  // Force dark mode for music app aesthetic
        }
    }
}
