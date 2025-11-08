import Foundation
import Combine
import GRUVBOKBridge

/// SwiftUI-compatible wrapper for EngineWrapper
/// Publishes state changes for reactive UI updates
class EngineState: ObservableObject {
    private let engineWrapper: EngineWrapper
    private var updateTimer: Timer?

    // Published state
    @Published var currentMode: Int = 0
    @Published var currentPattern: Int = 0
    @Published var currentTrack: Int = 0
    @Published var currentStep: Int = 0
    @Published var songModeStep: Int = 0  // Mode 0 step (1/16th speed)
    @Published var tempo: Int = 120
    @Published var isPlaying: Bool = false
    @Published var isDirty: Bool = false
    @Published var ledOn: Bool = false

    // Track data
    @Published var trackEvents: [EventData] = []

    // Pot values (for UI display)
    @Published var rotaryPots: [UInt8] = [0, 42, 0, 0]  // Mode, Tempo, Pattern, Track
    @Published var sliderPots: [UInt8] = [64, 64, 64, 64]  // S1-S4

    // Logs
    @Published var logMessages: [String] = []

    // Button debouncing
    @Published var pressedButtons: Set<Int> = []

    // State update throttling
    private var skipTrackEventsUpdate: Bool = false

    init(platform: String) {
        engineWrapper = EngineWrapper(platform: platform)

        if !engineWrapper.initialize() {
            print("ERROR: Failed to initialize engine")
        }

        // Set default rotary pot values
        engineWrapper.simulateRotaryPot(0, value: 0)   // Mode 0
        engineWrapper.simulateRotaryPot(1, value: 42)  // Tempo ~120 BPM
        engineWrapper.simulateRotaryPot(2, value: 0)   // Pattern 0
        engineWrapper.simulateRotaryPot(3, value: 0)   // Track 0

        // Load demo content
        engineWrapper.loadDemoContent()

        // Start update loop at 60fps
        startUpdateLoop()
    }

    func loadDemoContent() {
        engineWrapper.loadDemoContent()
    }

    func startUpdateLoop() {
        updateTimer = Timer.scheduledTimer(withTimeInterval: 1.0/60.0, repeats: true) { [weak self] _ in
            self?.update()
        }
    }

    func stopUpdateLoop() {
        updateTimer?.invalidate()
        updateTimer = nil
    }

    private func update() {
        // Call C++ engine update
        engineWrapper.update()

        // Poll state (efficient - just reading values)
        currentMode = Int(engineWrapper.getCurrentMode())
        currentPattern = Int(engineWrapper.getCurrentPattern())
        currentTrack = Int(engineWrapper.getCurrentTrack())
        currentStep = Int(engineWrapper.getCurrentStep())
        songModeStep = Int(engineWrapper.getSongModeStep())
        tempo = Int(engineWrapper.getTempo())
        isPlaying = engineWrapper.isPlaying()
        isDirty = engineWrapper.isDirty()
        ledOn = engineWrapper.getLEDState()

        // Update track events (skip during button press to avoid race condition)
        if !skipTrackEventsUpdate {
            trackEvents = engineWrapper.getTrackEvents()
        }

        // Update logs (throttled - only every 10 frames)
        if Int.random(in: 0...9) == 0 {
            logMessages = engineWrapper.getLogMessages()
        }
    }

    // MARK: - Control Methods

    func setMode(_ mode: Int) {
        currentMode = mode
        engineWrapper.setMode(mode)
    }

    func setTempo(_ tempo: Int) {
        self.tempo = tempo
        engineWrapper.setTempo(tempo)
    }

    func setPattern(_ pattern: Int) {
        currentPattern = pattern
        engineWrapper.setPattern(pattern)
    }

    func setTrack(_ track: Int) {
        currentTrack = track
        engineWrapper.setTrack(track)
    }

    func toggleButton(_ button: Int) {
        // Debounce: ignore if button already pressed
        guard !pressedButtons.contains(button) else { return }

        pressedButtons.insert(button)

        // Prevent state overwrite during button press
        skipTrackEventsUpdate = true

        engineWrapper.simulateButton(button, pressed: true)

        // Auto-release after 50ms (increased for more reliable registration)
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.05) { [weak self] in
            self?.engineWrapper.simulateButton(button, pressed: false)
            self?.pressedButtons.remove(button)

            // Wait another 50ms for C++ engine to process, then refresh state
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.05) { [weak self] in
                self?.trackEvents = self?.engineWrapper.getTrackEvents() ?? []
                self?.skipTrackEventsUpdate = false
            }
        }
    }

    func setRotaryPot(_ pot: Int, value: UInt8) {
        if pot >= 0 && pot < 4 {
            rotaryPots[pot] = value
            engineWrapper.simulateRotaryPot(pot, value: value)

            // Update corresponding state
            switch pot {
            case 0: setMode(Int(value) * 15 / 128)
            case 1: setTempo(60 + Int(value) * 180 / 127)
            case 2: setPattern(Int(value) * 32 / 128)
            case 3: setTrack(Int(value) * 8 / 128)
            default: break
            }
        }
    }

    func setSliderPot(_ pot: Int, value: UInt8) {
        if pot >= 0 && pot < 4 {
            sliderPots[pot] = value
            engineWrapper.simulateSliderPot(pot, value: value)
        }
    }

    // MARK: - Audio Control

    func setUseInternalAudio(_ enabled: Bool) {
        engineWrapper.setUseInternalAudio(enabled)
    }

    func setUseExternalMIDI(_ enabled: Bool) {
        engineWrapper.setUseExternalMIDI(enabled)
    }

    func setAudioGain(_ gain: Float) {
        engineWrapper.setAudioGain(gain)
    }

    var isAudioReady: Bool {
        engineWrapper.isAudioReady()
    }

    // MARK: - MIDI Output Port Management

    var midiOutputCount: Int {
        Int(engineWrapper.getMidiOutputCount())
    }

    func midiOutputName(at index: Int) -> String {
        engineWrapper.getMidiOutputName(index)
    }

    func selectMidiOutput(at index: Int) -> Bool {
        engineWrapper.selectMidiOutput(index)
    }

    // MARK: - MIDI Input Port Management (Mirror Mode)

    var midiInputCount: Int {
        Int(engineWrapper.getMidiInputCount())
    }

    func midiInputName(at index: Int) -> String {
        engineWrapper.getMidiInputName(index)
    }

    func selectMidiInput(at index: Int) -> Bool {
        engineWrapper.selectMidiInput(index)
    }

    var currentMidiInput: Int {
        Int(engineWrapper.getCurrentMidiInput())
    }

    var isMirrorModeEnabled: Bool {
        engineWrapper.isMirrorModeEnabled()
    }

    func setMirrorMode(_ enabled: Bool) {
        engineWrapper.setMirrorMode(enabled)
    }

    // MARK: - Song Persistence

    func saveSong(path: String, name: String) -> Bool {
        triggerLEDPattern("SAVING")
        let success = engineWrapper.saveSong(toPath: path, name: name)
        if !success {
            triggerLEDPattern("ERROR")
        }
        return success
    }

    func loadSong(path: String) -> (success: Bool, name: String?, tempo: Int?) {
        triggerLEDPattern("LOADING")
        var name: NSString? = nil
        var tempo: Int = 120

        let success = engineWrapper.loadSong(fromPath: path, outName: &name, outTempo: &tempo)

        if success {
            triggerLEDPattern("TEMPO_BEAT")
            return (true, name as String?, tempo)
        } else {
            triggerLEDPattern("ERROR")
            return (false, nil, nil)
        }
    }

    func clearLog() {
        engineWrapper.clearLog()
        logMessages = []
    }

    // MARK: - Lua Mode Management

    func reloadMode(_ mode: Int) -> Bool {
        return engineWrapper.reloadMode(mode)
    }

    func triggerLEDPattern(_ pattern: String) {
        engineWrapper.triggerLEDPattern(pattern)
    }

    deinit {
        stopUpdateLoop()
        engineWrapper.stop()
    }
}
