import SwiftUI
import AppKit
import Highlightr

struct CodeEditorView: NSViewRepresentable {
    @Binding var text: String
    let language: String

    func makeNSView(context: Context) -> NSScrollView {
        let scrollView = NSTextView.scrollableTextView()
        let textView = scrollView.documentView as! NSTextView

        // Configure text view
        textView.font = NSFont.monospacedSystemFont(ofSize: 13, weight: .regular)
        textView.autoresizingMask = [.width, .height]
        textView.isAutomaticQuoteSubstitutionEnabled = false
        textView.isAutomaticDashSubstitutionEnabled = false
        textView.isAutomaticTextReplacementEnabled = false
        textView.isAutomaticSpellingCorrectionEnabled = false
        textView.delegate = context.coordinator

        // Setup syntax highlighting
        if let highlightr = Highlightr() {
            highlightr.setTheme(to: "monokai")
            context.coordinator.highlightr = highlightr
            context.coordinator.textView = textView

            // Apply initial highlighting
            if let highlighted = highlightr.highlight(text, as: language) {
                textView.textStorage?.setAttributedString(highlighted)
            } else {
                textView.string = text
            }
        } else {
            textView.string = text
        }

        // Dark background
        textView.backgroundColor = NSColor(white: 0.05, alpha: 1.0)
        textView.insertionPointColor = .green

        return scrollView
    }

    func updateNSView(_ nsView: NSScrollView, context: Context) {
        let textView = nsView.documentView as! NSTextView

        // Only update if text changed from outside
        if textView.string != text {
            if let highlightr = context.coordinator.highlightr,
               let highlighted = highlightr.highlight(text, as: language) {
                textView.textStorage?.setAttributedString(highlighted)
            } else {
                textView.string = text
            }
        }
    }

    func makeCoordinator() -> Coordinator {
        Coordinator(self)
    }

    class Coordinator: NSObject, NSTextViewDelegate {
        var parent: CodeEditorView
        var highlightr: Highlightr?
        weak var textView: NSTextView?

        init(_ parent: CodeEditorView) {
            self.parent = parent
        }

        func textDidChange(_ notification: Notification) {
            guard let textView = notification.object as? NSTextView else { return }

            // Update binding
            parent.text = textView.string

            // Re-highlight
            if let highlightr = highlightr {
                let cursorPosition = textView.selectedRange().location

                if let highlighted = highlightr.highlight(textView.string, as: parent.language) {
                    textView.textStorage?.setAttributedString(highlighted)

                    // Restore cursor position
                    textView.setSelectedRange(NSRange(location: min(cursorPosition, textView.string.count), length: 0))
                }
            }
        }
    }
}
