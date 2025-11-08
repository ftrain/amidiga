#import "MacOSHardware.h"
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreAudio/CoreAudio.h>
#import <mach/mach_time.h>

namespace gruvbok {

MacOSHardware::MacOSHardware()
    : midi_client_(0)
    , midi_output_port_(0)
    , midi_virtual_source_(0)
    , current_midi_output_(-1)
    , midi_initialized_(false)
    , use_external_midi_(true)
    , midi_input_port_(0)
    , current_midi_input_(-1)
    , mirror_mode_enabled_(false)
    , audio_engine_(nullptr)
    , sampler_(nullptr)
    , audio_initialized_(false)
    , use_internal_audio_(false)
    , led_state_(false) {

    buttons_.fill(false);
    rotary_pots_.fill(64);  // Default middle value
    slider_pots_.fill(64);
}

MacOSHardware::~MacOSHardware() {
    shutdown();
}

bool MacOSHardware::init() {
    start_time_ = mach_absolute_time();

    // Initialize CoreMIDI
    OSStatus status = MIDIClientCreate(CFSTR("GRUVBOK"), nullptr, nullptr, &midi_client_);
    if (status != noErr) {
        addLog("ERROR: Failed to create MIDI client");
        return false;
    }

    // Create output port
    status = MIDIOutputPortCreate(midi_client_, CFSTR("GRUVBOK Output"), &midi_output_port_);
    if (status != noErr) {
        addLog("ERROR: Failed to create MIDI output port");
        return false;
    }

    // Create virtual source (for IAC Driver, etc.)
    status = MIDISourceCreate(midi_client_, CFSTR("GRUVBOK Virtual"), &midi_virtual_source_);
    if (status != noErr) {
        addLog("WARNING: Failed to create virtual MIDI source");
    } else {
        addLog("✓ Created virtual MIDI source 'GRUVBOK Virtual'");
    }

    midi_initialized_ = true;
    addLog("✓ CoreMIDI initialized");

    return true;
}

bool MacOSHardware::initAudio(const std::string& soundfont_path) {
    @autoreleasepool {
        audio_engine_ = [[AVAudioEngine alloc] init];
        sampler_ = [[AVAudioUnitSampler alloc] init];

        // Attach sampler to audio engine
        [audio_engine_ attachNode:sampler_];

        // Connect sampler to main mixer
        AVAudioFormat* format = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100.0
                                                                                channels:2];
        [audio_engine_ connect:sampler_
                            to:[audio_engine_ mainMixerNode]
                        format:format];

        // Load SoundFont
        NSURL* sfURL = nil;

        if (!soundfont_path.empty()) {
            // Try provided path
            sfURL = [NSURL fileURLWithPath:@(soundfont_path.c_str())];
        } else {
            // Try bundle resource
            sfURL = [[NSBundle mainBundle] URLForResource:@"default" withExtension:@"sf2"];
        }

        if (!sfURL || ![[NSFileManager defaultManager] fileExistsAtPath:[sfURL path]]) {
            addLog("ERROR: SoundFont not found");
            // Try to use Apple's built-in synth instead
            addLog("Attempting to use built-in DLS instrument...");

            // Load default instrument (General MIDI piano)
            // Note: On macOS, we need a SoundFont file - built-in DLS not available via AVAudioUnitSampler
            addLog("WARNING: No SoundFont found and built-in DLS not available on macOS");
            addLog("Audio playback will not work. Please provide a SoundFont file.");
            // Continue anyway to allow MIDI output testing
            audio_initialized_ = false;
            return true;  // Return true to allow MIDI-only operation
        } else {
            // Load the entire SoundFont bank (all instruments + drums)
            NSError* error = nil;

            // Load the soundfont with default melodic bank
            // For General MIDI: bankMSB=0x79 (121), bankLSB=0x00
            if (![sampler_ loadSoundBankInstrumentAtURL:sfURL
                                                program:0
                                                 bankMSB:0x79  // GM Level 1 melodic bank
                                                 bankLSB:kAUSampler_DefaultBankLSB
                                                  error:&error]) {
                addLog("ERROR: Failed to load SoundFont: " + std::string([[error localizedDescription] UTF8String]));
                return false;
            }

            addLog("✓ Loaded SoundFont: " + std::string([[sfURL lastPathComponent] UTF8String]));

            // Multi-timbral mode: AVAudioUnitSampler responds to all 16 MIDI channels
            // Channel 10 (index 9) is automatically drums in General MIDI soundfonts
            addLog("✓ Multi-timbral mode enabled (16 channels)");
        }

        // Start audio engine
        NSError* error = nil;
        if (![audio_engine_ startAndReturnError:&error]) {
            addLog("ERROR: Failed to start audio engine: " + std::string([[error localizedDescription] UTF8String]));
            return false;
        }

        audio_initialized_ = true;
        addLog("✓ AVAudioEngine started");
        return true;
    }
}

void MacOSHardware::shutdown() {
    if (audio_engine_) {
        [audio_engine_ stop];
        audio_engine_ = nullptr;
    }

    if (midi_virtual_source_) {
        MIDIEndpointDispose(midi_virtual_source_);
        midi_virtual_source_ = 0;
    }

    if (midi_output_port_) {
        MIDIPortDispose(midi_output_port_);
        midi_output_port_ = 0;
    }

    if (midi_client_) {
        MIDIClientDispose(midi_client_);
        midi_client_ = 0;
    }
}

bool MacOSHardware::readButton(int button) {
    if (button < 0 || button >= 16) return false;
    return buttons_[button];
}

uint8_t MacOSHardware::readRotaryPot(int pot) {
    if (pot < 0 || pot >= 4) return 0;
    return rotary_pots_[pot];
}

uint8_t MacOSHardware::readSliderPot(int pot) {
    if (pot < 0 || pot >= 4) return 0;
    return slider_pots_[pot];
}

void MacOSHardware::sendMidiMessage(const MidiMessage& msg) {
    if (use_external_midi_ && midi_initialized_) {
        sendToCoreMIDI(msg);
    }

    if (use_internal_audio_ && audio_initialized_) {
        sendToAudioEngine(msg);
    }
}

void MacOSHardware::sendToCoreMIDI(const MidiMessage& msg) {
    if (!midi_initialized_ || msg.data.empty()) return;

    // Send to virtual source
    if (midi_virtual_source_) {
        Byte buffer[1024];
        MIDIPacketList* packetList = (MIDIPacketList*)buffer;
        MIDIPacket* packet = MIDIPacketListInit(packetList);

        MIDITimeStamp timestamp = mach_absolute_time();
        packet = MIDIPacketListAdd(packetList, sizeof(buffer), packet,
                                   timestamp, msg.data.size(), msg.data.data());

        if (packet) {
            MIDIReceived(midi_virtual_source_, packetList);
        }
    }

    // Send to selected output if any
    if (current_midi_output_ >= 0) {
        ItemCount destCount = MIDIGetNumberOfDestinations();
        if (current_midi_output_ < (int)destCount) {
            MIDIEndpointRef dest = MIDIGetDestination(current_midi_output_);

            Byte buffer[1024];
            MIDIPacketList* packetList = (MIDIPacketList*)buffer;
            MIDIPacket* packet = MIDIPacketListInit(packetList);

            MIDITimeStamp timestamp = mach_absolute_time();
            packet = MIDIPacketListAdd(packetList, sizeof(buffer), packet,
                                       timestamp, msg.data.size(), msg.data.data());

            if (packet) {
                MIDISend(midi_output_port_, dest, packetList);
            }
        }
    }
}

void MacOSHardware::sendToAudioEngine(const MidiMessage& msg) {
    @autoreleasepool {
        if (!audio_initialized_ || !sampler_ || msg.data.empty()) return;

        UInt8 status = msg.data[0];
        UInt8 channel = status & 0x0F;
        UInt8 command = status & 0xF0;

        if (command == 0x90 && msg.data.size() >= 3) {  // Note On
            UInt8 note = msg.data[1];
            UInt8 velocity = msg.data[2];
            if (velocity > 0) {
                [sampler_ startNote:note withVelocity:velocity onChannel:channel];
            } else {
                [sampler_ stopNote:note onChannel:channel];
            }
        } else if (command == 0x80 && msg.data.size() >= 3) {  // Note Off
            UInt8 note = msg.data[1];
            [sampler_ stopNote:note onChannel:channel];
        } else if (command == 0xC0 && msg.data.size() >= 2) {  // Program Change
            UInt8 program = msg.data[1];
            // Send program change via raw MIDI to sampler
            // AVAudioUnitSampler handles GM channel mapping internally
            UInt8 status = 0xC0 | channel;
            [sampler_ sendMIDIEvent:status data1:program];
        } else if (command == 0xB0 && msg.data.size() >= 3) {  // Control Change
            UInt8 controller = msg.data[1];
            UInt8 value = msg.data[2];
            UInt8 status = 0xB0 | channel;
            [sampler_ sendMIDIEvent:status data1:controller data2:value];
        }
    }
}

void MacOSHardware::setLED(bool on) {
    led_state_ = on;
}

uint32_t MacOSHardware::getMillis() {
    static mach_timebase_info_data_t timebase;
    if (timebase.denom == 0) {
        mach_timebase_info(&timebase);
    }

    uint64_t now = mach_absolute_time();
    uint64_t elapsed = now - start_time_;
    uint64_t nanos = elapsed * timebase.numer / timebase.denom;
    return static_cast<uint32_t>(nanos / 1000000);
}

void MacOSHardware::update() {
    // Nothing to update for macOS (no polling needed)
}

void MacOSHardware::simulateButton(int button, bool pressed) {
    if (button >= 0 && button < 16) {
        buttons_[button] = pressed;
    }
}

void MacOSHardware::simulateRotaryPot(int pot, uint8_t value) {
    if (pot >= 0 && pot < 4) {
        rotary_pots_[pot] = std::min(value, static_cast<uint8_t>(127));
    }
}

void MacOSHardware::simulateSliderPot(int pot, uint8_t value) {
    if (pot >= 0 && pot < 4) {
        slider_pots_[pot] = std::min(value, static_cast<uint8_t>(127));
    }
}

void MacOSHardware::setAudioGain(float gain) {
    if (audio_engine_) {
        [audio_engine_ mainMixerNode].outputVolume = gain;
    }
}

void MacOSHardware::addLog(const std::string& message) {
    log_messages_.push_back(message);
    if (log_messages_.size() > MAX_LOG_MESSAGES) {
        log_messages_.pop_front();
    }
}

void MacOSHardware::clearLog() {
    log_messages_.clear();
}

int MacOSHardware::getMidiOutputCount() {
    return (int)MIDIGetNumberOfDestinations();
}

std::string MacOSHardware::getMidiOutputName(int index) {
    if (index < 0 || index >= getMidiOutputCount()) {
        return "";
    }

    MIDIEndpointRef dest = MIDIGetDestination(index);
    CFStringRef name = nullptr;
    MIDIObjectGetStringProperty(dest, kMIDIPropertyName, &name);

    if (name) {
        std::string result = [(__bridge NSString*)name UTF8String];
        CFRelease(name);
        return result;
    }

    return "Unknown";
}

bool MacOSHardware::selectMidiOutput(int index) {
    if (index < 0 || index >= getMidiOutputCount()) {
        current_midi_output_ = -1;
        return false;
    }

    current_midi_output_ = index;
    addLog("Selected MIDI output: " + getMidiOutputName(index));
    return true;
}

// MIDI Input (Mirror Mode) implementation

int MacOSHardware::getMidiInputCount() {
    return (int)MIDIGetNumberOfSources();
}

std::string MacOSHardware::getMidiInputName(int index) {
    if (index < 0 || index >= getMidiInputCount()) {
        return "";
    }

    MIDIEndpointRef source = MIDIGetSource(index);
    CFStringRef name = nullptr;
    MIDIObjectGetStringProperty(source, kMIDIPropertyName, &name);

    if (name) {
        std::string result = [(__bridge NSString*)name UTF8String];
        CFRelease(name);
        return result;
    }

    return "Unknown";
}

// MIDI input callback
static void MidiInputCallback(const MIDIPacketList *pktlist, void *readProcRefCon, void *srcConnRefCon) {
    MacOSHardware* hardware = (MacOSHardware*)readProcRefCon;
    if (!hardware || !hardware->isMirrorModeEnabled()) {
        return;
    }

    // Forward MIDI input to output (mirror/passthrough)
    const MIDIPacket *packet = &pktlist->packet[0];
    for (unsigned int i = 0; i < pktlist->numPackets; i++) {
        // Create MIDI message from packet data
        if (packet->length >= 1) {
            std::vector<uint8_t> data(packet->data, packet->data + packet->length);
            MidiMessage msg(data, 0);  // timestamp will be set by sendMidiMessage

            // Send to output
            hardware->sendMidiMessage(msg);
        }

        packet = MIDIPacketNext(packet);
    }
}

bool MacOSHardware::selectMidiInput(int index) {
    if (index < 0 || index >= getMidiInputCount()) {
        current_midi_input_ = -1;
        return false;
    }

    // Disconnect previous input if any
    if (midi_input_port_ && current_midi_input_ >= 0) {
        MIDIEndpointRef prev_source = MIDIGetSource(current_midi_input_);
        MIDIPortDisconnectSource(midi_input_port_, prev_source);
    }

    // Create input port if needed
    if (!midi_input_port_) {
        OSStatus status = MIDIInputPortCreate(midi_client_, CFSTR("GRUVBOK Input"),
                                              MidiInputCallback, this, &midi_input_port_);
        if (status != noErr) {
            addLog("ERROR: Failed to create MIDI input port");
            return false;
        }
    }

    // Connect to new source
    MIDIEndpointRef source = MIDIGetSource(index);
    OSStatus status = MIDIPortConnectSource(midi_input_port_, source, nullptr);

    if (status != noErr) {
        addLog("ERROR: Failed to connect MIDI input source");
        current_midi_input_ = -1;
        return false;
    }

    current_midi_input_ = index;
    addLog("Selected MIDI input: " + getMidiInputName(index));
    return true;
}

void MacOSHardware::setMirrorMode(bool enabled) {
    mirror_mode_enabled_ = enabled;

    if (enabled) {
        addLog("✓ Mirror Mode enabled");
    } else {
        addLog("✗ Mirror Mode disabled");
    }
}

} // namespace gruvbok
