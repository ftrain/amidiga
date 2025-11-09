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
    , audio_engine_(nullptr)
    , sampler_(nullptr)
    , audio_initialized_(false)
    , use_internal_audio_(false)
    , macos_start_time_(0) {
    // Base class constructor initializes buttons_, pots_, led_state_, start_time_
}

MacOSHardware::~MacOSHardware() {
    shutdown();
}

bool MacOSHardware::init() {
    macos_start_time_ = mach_absolute_time();

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
            NSError* error = nil;
            if (![sampler_ loadInstrumentAtURL:nil
                                       program:0
                                        bankMSB:kAUSampler_DefaultMelodicBankMSB
                                        bankLSB:kAUSampler_DefaultBankLSB
                                         error:&error]) {
                addLog("ERROR: Failed to load instrument: " + std::string([[error localizedDescription] UTF8String]));
                return false;
            }
            addLog("✓ Loaded built-in DLS instrument");
        } else {
            // Load the SoundFont
            NSError* error = nil;
            if (![sampler_ loadSoundBankInstrumentAtURL:sfURL
                                                program:0
                                                 bankMSB:kAUSampler_DefaultMelodicBankMSB
                                                 bankLSB:kAUSampler_DefaultBankLSB
                                                  error:&error]) {
                addLog("ERROR: Failed to load SoundFont: " + std::string([[error localizedDescription] UTF8String]));
                return false;
            }
            addLog("✓ Loaded SoundFont: " + std::string([[sfURL lastPathComponent] UTF8String]));
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

// Note: readButton(), readRotaryPot(), readSliderPot(), setLED(), getLED(),
//       simulateButton(), simulateRotaryPot(), simulateSliderPot()
//       are now inherited from HardwareBase

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
        if (!audio_initialized_ || !sampler_ || msg.data.size() < 3) return;

        UInt8 status = msg.data[0];
        UInt8 byte1 = msg.data[1];
        UInt8 byte2 = msg.data[2];
        UInt8 channel = status & 0x0F;
        UInt8 command = status & 0xF0;

        if (command == 0x90 && byte2 > 0) {  // Note On
            [sampler_ startNote:byte1 withVelocity:byte2 onChannel:channel];
        } else if (command == 0x80 || (command == 0x90 && byte2 == 0)) {  // Note Off
            [sampler_ stopNote:byte1 onChannel:channel];
        }
        // TODO: Handle CC messages for filter, etc.
    }
}

uint32_t MacOSHardware::getMillis() {
    static mach_timebase_info_data_t timebase;
    if (timebase.denom == 0) {
        mach_timebase_info(&timebase);
    }

    uint64_t now = mach_absolute_time();
    uint64_t elapsed = now - macos_start_time_;
    uint64_t nanos = elapsed * timebase.numer / timebase.denom;
    return static_cast<uint32_t>(nanos / 1000000);
}

void MacOSHardware::update() {
    // Nothing to update for macOS (no polling needed)
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

} // namespace gruvbok
