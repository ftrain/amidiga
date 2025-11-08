import SwiftUI
import UniformTypeIdentifiers

struct OutputView: View {
    @ObservedObject var engine: EngineState
    @State private var selectedMidiPort: Int = -1
    @State private var selectedMidiInputPort: Int = -1
    @State private var mirrorModeEnabled: Bool = false
    @State private var showingSaveDialog = false
    @State private var showingLoadDialog = false
    @State private var songName: String = "My GRUVBOK Song"
    @State private var useExternalMIDI: Bool = true
    @State private var useInternalAudio: Bool = false
    @State private var audioGain: Float = 0.5

    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 12) {
                // MIDI Configuration Section
                midiConfigSection
                    .padding(10)
                    .background(
                        RoundedRectangle(cornerRadius: 8)
                            .fill(Color(white: 0.1))
                            .overlay(
                                RoundedRectangle(cornerRadius: 8)
                                    .stroke(Color.cyan.opacity(0.2), lineWidth: 1)
                            )
                    )

                // Audio Configuration Section
                audioConfigSection
                    .padding(10)
                    .background(
                        RoundedRectangle(cornerRadius: 8)
                            .fill(Color(white: 0.1))
                            .overlay(
                                RoundedRectangle(cornerRadius: 8)
                                    .stroke(Color.cyan.opacity(0.2), lineWidth: 1)
                            )
                    )

                // Song Persistence Section
                songPersistenceSection
                    .padding(10)
                    .background(
                        RoundedRectangle(cornerRadius: 8)
                            .fill(Color(white: 0.1))
                            .overlay(
                                RoundedRectangle(cornerRadius: 8)
                                    .stroke(Color.cyan.opacity(0.2), lineWidth: 1)
                            )
                    )
            }
            .padding(12)
        }
        .background(Color.black)
    }

    // MARK: - MIDI Configuration

    private var midiConfigSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("MIDI Configuration")
                .font(.system(size: 14, weight: .bold, design: .rounded))
                .foregroundColor(.cyan)
                .textCase(.uppercase)

            // MIDI Output
            HStack(spacing: 10) {
                Text("Output:")
                    .font(.system(size: 12))
                    .frame(width: 80, alignment: .trailing)

                Picker("MIDI Output", selection: $selectedMidiPort) {
                    Text("Virtual Port").tag(-1)
                    ForEach(0..<engine.midiOutputCount, id: \.self) { index in
                        Text(engine.midiOutputName(at: index)).tag(index)
                    }
                }
                .labelsHidden()
                .frame(width: 250)
                .onChange(of: selectedMidiPort) { _, newPort in
                    _ = engine.selectMidiOutput(at: newPort)
                }

                Toggle("Enable", isOn: $useExternalMIDI)
                    .onChange(of: useExternalMIDI) { _, enabled in
                        engine.setUseExternalMIDI(enabled)
                    }
                    .frame(width: 100)
            }

            // Mirror Mode
            HStack(spacing: 10) {
                Text("Mirror:")
                    .font(.system(size: 12))
                    .frame(width: 80, alignment: .trailing)

                Toggle("Enable MIDI Input Passthrough", isOn: $mirrorModeEnabled)
                    .onChange(of: mirrorModeEnabled) { _, enabled in
                        engine.setMirrorMode(enabled)
                        if enabled {
                            engine.triggerLEDPattern("MIRROR_MODE")
                        } else {
                            engine.triggerLEDPattern("TEMPO_BEAT")
                        }
                    }
                    .frame(width: 250)
            }

            // MIDI Input (shown when mirror mode is enabled)
            if mirrorModeEnabled {
                HStack(spacing: 10) {
                    Text("Input:")
                        .font(.system(size: 12))
                        .frame(width: 80, alignment: .trailing)

                    Picker("MIDI Input", selection: $selectedMidiInputPort) {
                        Text("Select Input...").tag(-1)
                        ForEach(0..<engine.midiInputCount, id: \.self) { index in
                            Text(engine.midiInputName(at: index)).tag(index)
                        }
                    }
                    .labelsHidden()
                    .frame(width: 250)
                    .onChange(of: selectedMidiInputPort) { _, newPort in
                        if newPort >= 0 {
                            _ = engine.selectMidiInput(at: newPort)
                        }
                    }
                }
            }
        }
    }

    // MARK: - Audio Configuration

    private var audioConfigSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Audio Output")
                .font(.system(size: 14, weight: .bold, design: .rounded))
                .foregroundColor(.cyan)
                .textCase(.uppercase)

            HStack(spacing: 10) {
                Text("Internal:")
                    .font(.system(size: 12))
                    .frame(width: 80, alignment: .trailing)

                Toggle("Enable FluidSynth", isOn: $useInternalAudio)
                    .onChange(of: useInternalAudio) { _, enabled in
                        engine.setUseInternalAudio(enabled)
                        // Note: Would need to initialize audio with SoundFont path
                    }
                    .frame(width: 160)

                if engine.isAudioReady {
                    Image(systemName: "checkmark.circle.fill")
                        .font(.caption)
                        .foregroundColor(.green)
                    Text("READY")
                        .font(.system(size: 11, weight: .semibold))
                        .foregroundColor(.green)
                } else if useInternalAudio {
                    Image(systemName: "xmark.circle.fill")
                        .font(.caption)
                        .foregroundColor(.red)
                    Text("FAILED")
                        .font(.system(size: 11, weight: .semibold))
                        .foregroundColor(.red)
                }
            }

            if engine.isAudioReady {
                HStack(spacing: 10) {
                    Text("Volume:")
                        .font(.system(size: 12))
                        .frame(width: 80, alignment: .trailing)

                    Slider(value: $audioGain, in: 0...2.0)
                        .frame(width: 200)
                        .onChange(of: audioGain) { _, gain in
                            engine.setAudioGain(gain)
                        }

                    Text(String(format: "%.2f", audioGain))
                        .font(.system(size: 11))
                        .foregroundColor(.secondary)
                        .frame(width: 50)
                }
            }
        }
    }

    // MARK: - Song Persistence

    private var songPersistenceSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("Song Persistence")
                .font(.system(size: 14, weight: .bold, design: .rounded))
                .foregroundColor(.cyan)
                .textCase(.uppercase)

            HStack(spacing: 10) {
                Text("Name:")
                    .font(.system(size: 12))
                    .frame(width: 80, alignment: .trailing)

                TextField("Song Name", text: $songName)
                    .textFieldStyle(.roundedBorder)
                    .frame(width: 250)
            }

            HStack(spacing: 10) {
                Spacer()
                    .frame(width: 80)

                Button("Save...") {
                    showingSaveDialog = true
                }
                .fileExporter(
                    isPresented: $showingSaveDialog,
                    document: JSONDocument(songName: songName),
                    contentType: .json,
                    defaultFilename: "\(songName.replacingOccurrences(of: " ", with: "_")).json"
                ) { result in
                    switch result {
                    case .success(let url):
                        if engine.saveSong(path: url.path, name: songName) {
                            print("✓ Song saved: \(url.path)")
                        } else {
                            print("✗ Failed to save song")
                        }
                    case .failure(let error):
                        print("Save error: \(error.localizedDescription)")
                    }
                }

                Button("Load...") {
                    showingLoadDialog = true
                }
                .fileImporter(
                    isPresented: $showingLoadDialog,
                    allowedContentTypes: [.json],
                    allowsMultipleSelection: false
                ) { result in
                    switch result {
                    case .success(let urls):
                        guard let url = urls.first else { return }
                        let loadResult = engine.loadSong(path: url.path)
                        if loadResult.success {
                            if let name = loadResult.name {
                                songName = name
                            }
                            print("✓ Song loaded: \(url.path)")
                        } else {
                            print("✗ Failed to load song")
                        }
                    case .failure(let error):
                        print("Load error: \(error.localizedDescription)")
                    }
                }

                Button("Quick Save") {
                    let timestamp = Int(Date().timeIntervalSince1970)
                    let path = "/tmp/gruvbok_autosave_\(timestamp).json"
                    if engine.saveSong(path: path, name: songName) {
                        print("✓ Autosaved: \(path)")
                    } else {
                        print("✗ Autosave failed")
                    }
                }

                Button("Demo") {
                    engine.loadDemoContent()
                    songName = "Demo Song"
                }
                .foregroundColor(.blue)
            }
        }
    }
}

// Helper document type for file exporters
struct JSONDocument: FileDocument {
    static var readableContentTypes: [UTType] { [.json] }

    let songName: String

    init(songName: String) {
        self.songName = songName
    }

    init(configuration: ReadConfiguration) throws {
        songName = "Loaded Song"
    }

    func fileWrapper(configuration: WriteConfiguration) throws -> FileWrapper {
        // This is just a placeholder - actual save happens in the button handler
        let data = Data()
        return FileWrapper(regularFileWithContents: data)
    }
}

