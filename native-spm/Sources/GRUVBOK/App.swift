import SwiftUI
import AppKit

@main
struct GruvbokApp: App {
    init() {
        // Activate the app and bring it to front when launched
        DispatchQueue.main.async {
            NSApp.activate(ignoringOtherApps: true)
        }
    }

    var body: some Scene {
        WindowGroup {
            ContentView(platform: "macOS")
                .frame(minWidth: 800, minHeight: 600)
                .onAppear {
                    // Ensure window is key and app is active
                    NSApp.activate(ignoringOtherApps: true)
                    NSApp.windows.first?.makeKeyAndOrderFront(nil)
                }
        }
    }
}
