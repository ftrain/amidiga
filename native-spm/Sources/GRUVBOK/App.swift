import SwiftUI

@main
struct GruvbokApp: App {
    var body: some Scene {
        WindowGroup {
            ContentView(platform: "macOS")
                .frame(minWidth: 800, minHeight: 600)
        }
    }
}
