import SwiftUI
import Foundation

struct LuaManagerView: View {
    @ObservedObject var engine: EngineState
    @State private var luaFiles: [LuaFileInfo] = []
    @State private var selectedFile: LuaFileInfo?
    @State private var editorContent: String = ""
    @State private var modesDirectoryPath: String = ""

    var body: some View {
        HSplitView {
            // Left: File browser
            fileBrowserSection
                .frame(minWidth: 250, maxWidth: 400)

            // Right: Text editor
            textEditorSection
                .frame(minWidth: 400)
        }
        .background(Color.black)
        .onAppear {
            loadLuaFiles()
        }
    }

    // MARK: - File Browser

    private var fileBrowserSection: some View {
        VStack(alignment: .leading, spacing: 0) {
            // Header
            Text("Lua Modes")
                .font(.system(size: 16, weight: .bold, design: .rounded))
                .foregroundColor(.cyan)
                .textCase(.uppercase)
                .padding()

            Divider()

            // File list
            ScrollView {
                VStack(spacing: 1) {
                    ForEach(luaFiles) { file in
                        fileRow(file)
                    }
                }
            }

            Divider()

            // Footer with path
            VStack(alignment: .leading, spacing: 4) {
                Text("Modes Directory:")
                    .font(.system(size: 10, weight: .semibold))
                    .foregroundColor(.secondary)
                Text(modesDirectoryPath)
                    .font(.system(size: 11, design: .monospaced))
                    .foregroundColor(.gray)
                    .lineLimit(2)
            }
            .padding(8)
            .background(Color(white: 0.08))
        }
        .background(Color(white: 0.1))
    }

    private func fileRow(_ file: LuaFileInfo) -> some View {
        Button(action: {
            selectedFile = file
            loadFileContent(file)
        }) {
            HStack(spacing: 10) {
                // Mode indicator
                if let modeNumber = file.modeNumber {
                    Text("\(modeNumber)")
                        .font(.system(size: 12, weight: .bold, design: .monospaced))
                        .foregroundColor(modeNumber == engine.currentMode ? .cyan : .white)
                        .frame(width: 30, height: 30)
                        .background(
                            RoundedRectangle(cornerRadius: 4)
                                .fill(modeNumber == engine.currentMode ? Color.cyan.opacity(0.2) : Color(white: 0.15))
                        )
                }

                VStack(alignment: .leading, spacing: 2) {
                    Text(file.displayName)
                        .font(.system(size: 13, weight: .semibold))
                        .foregroundColor(.white)

                    if !file.sliderLabels.isEmpty {
                        Text(file.sliderLabels.joined(separator: " • "))
                            .font(.system(size: 13))
                            .foregroundColor(.cyan.opacity(0.7))
                            .lineLimit(1)
                    }
                }

                Spacer()

                // Active indicator
                if let modeNumber = file.modeNumber, modeNumber == engine.currentMode {
                    Circle()
                        .fill(Color.green)
                        .frame(width: 8, height: 8)
                        .shadow(color: Color.green.opacity(0.6), radius: 3)
                }
            }
            .padding(10)
            .background(
                RoundedRectangle(cornerRadius: 6)
                    .fill(selectedFile?.id == file.id ? Color.cyan.opacity(0.15) : Color.clear)
            )
        }
        .buttonStyle(.plain)
    }

    // MARK: - Text Editor

    private var textEditorSection: some View {
        VStack(alignment: .leading, spacing: 0) {
            // Header with file name and controls
            HStack {
                if let file = selectedFile {
                    Text(file.filename)
                        .font(.system(size: 14, weight: .semibold, design: .monospaced))
                        .foregroundColor(.cyan)
                } else {
                    Text("No file selected")
                        .font(.system(size: 14, weight: .semibold))
                        .foregroundColor(.secondary)
                }

                Spacer()

                if selectedFile != nil {
                    Button("Save") {
                        saveFile()
                    }
                    .buttonStyle(.borderedProminent)
                    .tint(.cyan)

                    Button("Reload Mode") {
                        reloadMode()
                    }
                    .buttonStyle(.bordered)
                }
            }
            .padding()
            .background(Color(white: 0.12))

            Divider()

            // Text editor with syntax highlighting
            if selectedFile != nil {
                CodeEditorView(text: $editorContent, language: "lua")
            } else {
                VStack {
                    Spacer()
                    Text("Select a Lua file to edit")
                        .font(.system(size: 14))
                        .foregroundColor(.secondary)
                    Spacer()
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
                .background(Color(white: 0.05))
            }
        }
        .background(Color(white: 0.08))
    }

    // MARK: - File Operations

    private func loadLuaFiles() {
        // Find modes directory
        let possiblePaths = [
            "/Users/ford/shared/dev/amidiga/modes",
            "/Users/ford/Library/Mobile Documents/com~apple~CloudDocs/shared/dev/amidiga/modes",
            // Try relative to executable
            Bundle.main.bundlePath + "/../../../../modes",
            Bundle.main.bundlePath + "/../../../modes",
            Bundle.main.bundlePath + "/../../modes",
            Bundle.main.bundlePath + "/../modes"
        ]

        for path in possiblePaths {
            let standardized = (path as NSString).standardizingPath
            if FileManager.default.fileExists(atPath: standardized) {
                modesDirectoryPath = standardized
                break
            }
        }

        guard !modesDirectoryPath.isEmpty else {
            print("Could not find modes directory")
            return
        }

        // Scan for .lua files
        do {
            let files = try FileManager.default.contentsOfDirectory(atPath: modesDirectoryPath)
            luaFiles = files
                .filter { $0.hasSuffix(".lua") }
                .compactMap { filename in
                    parseLuaFile(filename: filename, path: modesDirectoryPath + "/" + filename)
                }
                .sorted { $0.filename < $1.filename }
        } catch {
            print("Error loading Lua files: \(error)")
        }
    }

    private func parseLuaFile(filename: String, path: String) -> LuaFileInfo? {
        guard let content = try? String(contentsOfFile: path, encoding: .utf8) else {
            return nil
        }

        // Parse mode number from filename (e.g., "01_drums.lua" -> 1)
        let modeNumber: Int? = {
            let parts = filename.split(separator: "_")
            if let first = parts.first, let num = Int(first) {
                return num
            }
            return nil
        }()

        // Parse MODE_NAME
        let displayName: String = {
            let pattern = #"MODE_NAME\s*=\s*"([^"]+)""#
            if let regex = try? NSRegularExpression(pattern: pattern),
               let match = regex.firstMatch(in: content, range: NSRange(content.startIndex..., in: content)),
               let range = Range(match.range(at: 1), in: content) {
                return String(content[range])
            }
            return filename.replacingOccurrences(of: ".lua", with: "")
        }()

        // Parse SLIDER_LABELS
        let sliderLabels: [String] = {
            let pattern = #"SLIDER_LABELS\s*=\s*\{([^}]+)\}"#
            if let regex = try? NSRegularExpression(pattern: pattern),
               let match = regex.firstMatch(in: content, range: NSRange(content.startIndex..., in: content)),
               let range = Range(match.range(at: 1), in: content) {
                let labelsStr = String(content[range])
                return labelsStr.split(separator: ",")
                    .map { $0.trimmingCharacters(in: .whitespacesAndNewlines).replacingOccurrences(of: "\"", with: "") }
            }
            return []
        }()

        return LuaFileInfo(
            filename: filename,
            path: path,
            modeNumber: modeNumber,
            displayName: displayName,
            sliderLabels: sliderLabels
        )
    }

    private func loadFileContent(_ file: LuaFileInfo) {
        if let content = try? String(contentsOfFile: file.path, encoding: .utf8) {
            editorContent = content
        }
    }

    private func saveFile() {
        guard let file = selectedFile else { return }

        do {
            try editorContent.write(toFile: file.path, atomically: true, encoding: .utf8)
            print("Saved \(file.filename)")
        } catch {
            print("Error saving file: \(error)")
        }
    }

    private func reloadMode() {
        guard let file = selectedFile, let modeNumber = file.modeNumber else { return }

        let success = engine.reloadMode(modeNumber)
        if success {
            print("Successfully reloaded mode \(modeNumber)")
        } else {
            print("Failed to reload mode \(modeNumber)")
        }
    }
}

// MARK: - Data Models

struct LuaFileInfo: Identifiable {
    let id = UUID()
    let filename: String
    let path: String
    let modeNumber: Int?
    let displayName: String
    let sliderLabels: [String]
}
