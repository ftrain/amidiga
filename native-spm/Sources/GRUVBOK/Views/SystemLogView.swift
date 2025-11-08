import SwiftUI

struct SystemLogView: View {
    @ObservedObject var engine: EngineState

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            HStack {
                Text("System Log")
                    .font(.system(size: 18, weight: .bold, design: .rounded))
                    .foregroundColor(.cyan)

                Spacer()

                Button("Clear Log") {
                    engine.clearLog()
                }
            }
            .padding(.horizontal)

            Divider()

            ScrollViewReader { proxy in
                ScrollView {
                    VStack(alignment: .leading, spacing: 2) {
                        ForEach(Array(engine.logMessages.enumerated()), id: \.offset) { index, message in
                            Text(message)
                                .font(.system(size: 12, design: .monospaced))
                                .foregroundColor(.primary)
                                .textSelection(.enabled)
                                .id(index)
                        }
                    }
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding()
                }
                .onChange(of: engine.logMessages.count) { _ in
                    // Auto-scroll to bottom when new messages arrive
                    if let lastIndex = engine.logMessages.indices.last {
                        withAnimation {
                            proxy.scrollTo(lastIndex, anchor: .bottom)
                        }
                    }
                }
            }
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
}
