import SwiftUI

@main
struct GruvbokApp: App {
    var body: some Scene {
        WindowGroup {
            ContentView(platform: "macOS")
                .frame(minWidth: 900, minHeight: 700)
        }
        .windowStyle(.hiddenTitleBar)
        .windowResizability(.contentSize)
    }
}
