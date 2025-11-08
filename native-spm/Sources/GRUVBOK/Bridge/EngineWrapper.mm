#import "EngineWrapper.h"
#include "core/song.h"
#include "core/engine.h"
#include "lua_bridge/mode_loader.h"

#if TARGET_OS_IPHONE
#include "../iOS/IOSHardware.h"
#else
#include "../Hardware/MacOSHardware.h"
#endif

#include <memory>
#include <vector>
#include <string>
#include <iostream>

using namespace gruvbok;

// EventData implementation
@implementation EventData

- (instancetype)initWithStep:(NSInteger)step
                    switchOn:(BOOL)switchOn
                        pots:(NSArray<NSNumber *> *)pots {
    self = [super init];
    if (self) {
        _step = step;
        _switchOn = switchOn;
        _pots = pots;
    }
    return self;
}

@end

// EngineWrapper implementation
@interface EngineWrapper() {
    std::unique_ptr<Song> song_;
#if TARGET_OS_IPHONE
    std::unique_ptr<IOSHardware> hardware_;
#else
    std::unique_ptr<MacOSHardware> hardware_;
#endif
    std::unique_ptr<ModeLoader> modeLoader_;
    std::unique_ptr<Engine> engine_;
    NSString* platform_;
}
@end

@implementation EngineWrapper

- (instancetype)initWithPlatform:(NSString *)platform {
    self = [super init];
    if (self) {
        platform_ = platform;
        song_ = std::make_unique<Song>();
#if TARGET_OS_IPHONE
        hardware_ = std::make_unique<IOSHardware>();
#else
        hardware_ = std::make_unique<MacOSHardware>();
#endif
        modeLoader_ = std::make_unique<ModeLoader>();
    }
    return self;
}

- (BOOL)initialize {
    // Initialize hardware
    if (!hardware_->init()) {
        NSLog(@"Failed to initialize hardware");
        return NO;
    }

    // Load Lua modes from bundle or fallback paths
    NSString* modesPath = [[NSBundle mainBundle] pathForResource:@"modes" ofType:nil];
    if (!modesPath) {
        // Try Resources/modes for app bundle
        modesPath = [[NSBundle mainBundle] pathForResource:@"Resources/modes" ofType:nil];
    }

    if (!modesPath) {
        // For SPM builds, try relative to executable
        NSString* execPath = [[NSBundle mainBundle] executablePath];
        NSString* execDir = [execPath stringByDeletingLastPathComponent];

        // Try ../../../../modes (for .build/arm64-apple-macosx/debug/gruvbok-native -> amidiga/modes)
        modesPath = [[execDir stringByAppendingPathComponent:@"../../../../modes"] stringByStandardizingPath];
        if (![[NSFileManager defaultManager] fileExistsAtPath:modesPath]) {
            // Try ../../../modes
            modesPath = [[execDir stringByAppendingPathComponent:@"../../../modes"] stringByStandardizingPath];
            if (![[NSFileManager defaultManager] fileExistsAtPath:modesPath]) {
                // Try ../../modes
                modesPath = [[execDir stringByAppendingPathComponent:@"../../modes"] stringByStandardizingPath];
                if (![[NSFileManager defaultManager] fileExistsAtPath:modesPath]) {
                    // Try ../modes
                    modesPath = [[execDir stringByAppendingPathComponent:@"../modes"] stringByStandardizingPath];
                    if (![[NSFileManager defaultManager] fileExistsAtPath:modesPath]) {
                        modesPath = nil;
                    }
                }
            }
        }
    }

    if (modesPath) {
        int loaded = modeLoader_->loadModesFromDirectory([modesPath UTF8String], 120);
        NSLog(@"Loaded %d Lua modes from: %@", loaded, modesPath);
        hardware_->addLog("Loaded " + std::to_string(loaded) + " Lua modes from: " + std::string([modesPath UTF8String]));
    } else {
        NSLog(@"WARNING: modes/ directory not found in bundle or relative paths");
        hardware_->addLog("WARNING: modes/ directory not found!");
    }

    // Create engine
    engine_ = std::make_unique<Engine>(song_.get(), hardware_.get(), modeLoader_.get());

    // Initialize audio (try built-in first)
    NSString* sfPath = [[NSBundle mainBundle] pathForResource:@"default" ofType:@"sf2"];
    if (sfPath) {
        hardware_->initAudio([sfPath UTF8String]);
    } else {
        // Use built-in DLS
        hardware_->initAudio("");
    }

#if TARGET_OS_IPHONE
    // iOS defaults to internal audio
    hardware_->setUseInternalAudio(true);
    hardware_->setUseExternalMIDI(false);
#else
    // macOS can use both
    hardware_->setUseInternalAudio(true);
    hardware_->setUseExternalMIDI(true);
#endif

    // Start engine
    NSLog(@"Starting engine...");
    hardware_->addLog("Starting engine...");
    engine_->start();
    NSLog(@"Engine started. Is playing: %d", engine_->isPlaying() ? 1 : 0);
    hardware_->addLog(engine_->isPlaying() ? "✓ Engine is playing" : "✗ Engine failed to start");

    return YES;
}

- (void)start {
    if (engine_) {
        engine_->start();
    }
}

- (void)stop {
    if (engine_) {
        engine_->stop();
    }
}

- (void)update {
    if (engine_) {
        engine_->update();
    }
}

- (BOOL)isPlaying {
    return engine_ ? engine_->isPlaying() : NO;
}

- (BOOL)isDirty {
    return engine_ ? engine_->isDirty() : NO;
}

- (NSInteger)getCurrentMode {
    return engine_ ? engine_->getCurrentMode() : 0;
}

- (NSInteger)getCurrentPattern {
    return engine_ ? engine_->getCurrentPattern() : 0;
}

- (NSInteger)getCurrentTrack {
    return engine_ ? engine_->getCurrentTrack() : 0;
}

- (NSInteger)getCurrentStep {
    return engine_ ? engine_->getCurrentStep() : 0;
}

- (NSInteger)getSongModeStep {
    return engine_ ? engine_->getSongModeStep() : 0;
}

- (NSInteger)getTempo {
    return engine_ ? engine_->getTempo() : 120;
}

- (BOOL)getLEDState {
    return hardware_ ? hardware_->getLED() : NO;
}

- (void)setTempo:(NSInteger)tempo {
    if (engine_) {
        engine_->setTempo((int)tempo);
    }
}

- (void)setMode:(NSInteger)mode {
    if (engine_) {
        engine_->setMode((int)mode);
    }
}

- (void)setPattern:(NSInteger)pattern {
    if (engine_) {
        engine_->setPattern((int)pattern);
    }
}

- (void)setTrack:(NSInteger)track {
    if (engine_) {
        engine_->setTrack((int)track);
    }
}

- (void)simulateButton:(NSInteger)button pressed:(BOOL)pressed {
    if (hardware_) {
        hardware_->simulateButton((int)button, pressed);
    }
}

- (void)simulateRotaryPot:(NSInteger)pot value:(uint8_t)value {
    if (hardware_) {
        hardware_->simulateRotaryPot((int)pot, value);
    }
}

- (void)simulateSliderPot:(NSInteger)pot value:(uint8_t)value {
    if (hardware_) {
        hardware_->simulateSliderPot((int)pot, value);
    }
}

- (NSArray<EventData *> *)getTrackEvents {
    if (!song_ || !engine_) {
        return @[];
    }

    int mode = engine_->getCurrentMode();
    int pattern = engine_->getCurrentPattern();
    int track = engine_->getCurrentTrack();

    Mode& m = song_->getMode(mode);
    Pattern& p = m.getPattern(pattern);
    Track& t = p.getTrack(track);

    NSMutableArray<EventData *> *events = [NSMutableArray arrayWithCapacity:16];

    for (int step = 0; step < 16; step++) {
        const Event& e = t.getEvent(step);

        NSArray<NSNumber *> *pots = @[
            @(e.getPot(0)),
            @(e.getPot(1)),
            @(e.getPot(2)),
            @(e.getPot(3))
        ];

        EventData *data = [[EventData alloc] initWithStep:step
                                                 switchOn:e.getSwitch()
                                                     pots:pots];
        [events addObject:data];
    }

    return events;
}

- (NSArray<NSString *> *)getLogMessages {
    if (!hardware_) {
        return @[];
    }

    const auto& messages = hardware_->getLogMessages();
    NSMutableArray<NSString *> *result = [NSMutableArray arrayWithCapacity:messages.size()];

    for (const auto& msg : messages) {
        [result addObject:@(msg.c_str())];
    }

    return result;
}

- (void)clearLog {
    if (hardware_) {
        hardware_->clearLog();
    }
}

- (BOOL)initAudioWithSoundFont:(nullable NSString *)soundfontPath {
    if (!hardware_) return NO;

    std::string path = soundfontPath ? [soundfontPath UTF8String] : "";
    return hardware_->initAudio(path);
}

- (void)setUseInternalAudio:(BOOL)useInternal {
    if (hardware_) {
        hardware_->setUseInternalAudio(useInternal);
    }
}

- (void)setUseExternalMIDI:(BOOL)useExternal {
    if (hardware_) {
        hardware_->setUseExternalMIDI(useExternal);
    }
}

- (BOOL)isAudioReady {
    return hardware_ ? hardware_->isAudioReady() : NO;
}

- (void)setAudioGain:(float)gain {
    if (hardware_) {
        hardware_->setAudioGain(gain);
    }
}

- (NSInteger)getMidiOutputCount {
    return hardware_ ? hardware_->getMidiOutputCount() : 0;
}

- (NSString *)getMidiOutputName:(NSInteger)index {
    if (!hardware_) return @"";
    std::string name = hardware_->getMidiOutputName((int)index);
    return @(name.c_str());
}

- (BOOL)selectMidiOutput:(NSInteger)index {
    return hardware_ ? hardware_->selectMidiOutput((int)index) : NO;
}

- (NSInteger)getCurrentMidiOutput {
    // MacOSHardware doesn't expose current port getter in original implementation
    return -1;
}

- (NSInteger)getMidiInputCount {
    return hardware_ ? hardware_->getMidiInputCount() : 0;
}

- (NSString *)getMidiInputName:(NSInteger)index {
    if (!hardware_) return @"";
    std::string name = hardware_->getMidiInputName((int)index);
    return @(name.c_str());
}

- (BOOL)selectMidiInput:(NSInteger)index {
    return hardware_ ? hardware_->selectMidiInput((int)index) : NO;
}

- (NSInteger)getCurrentMidiInput {
    return hardware_ ? hardware_->getCurrentMidiInput() : -1;
}

- (BOOL)isMirrorModeEnabled {
    return hardware_ ? hardware_->isMirrorModeEnabled() : NO;
}

- (void)setMirrorMode:(BOOL)enabled {
    if (hardware_) {
        hardware_->setMirrorMode(enabled);
    }
}

- (BOOL)saveSongToPath:(NSString *)path name:(NSString *)name {
    if (!song_ || !engine_) return NO;

    std::string cppPath = [path UTF8String];
    std::string cppName = [name UTF8String];
    int tempo = engine_->getTempo();

    if (song_->save(cppPath, cppName, tempo)) {
        engine_->clearDirty();
        return YES;
    }
    return NO;
}

- (BOOL)loadSongFromPath:(NSString *)path
                 outName:(NSString **)outName
               outTempo:(NSInteger *)outTempo {
    if (!song_ || !engine_) return NO;

    std::string cppPath = [path UTF8String];
    std::string loadedName;
    int loadedTempo = 120;

    bool success = song_->load(cppPath, &loadedName, &loadedTempo);

    if (success) {
        if (outName) {
            *outName = @(loadedName.c_str());
        }
        if (outTempo) {
            *outTempo = loadedTempo;
        }

        // Apply loaded tempo to engine
        engine_->setTempo(loadedTempo);

        // Clear dirty flag after successful load
        engine_->clearDirty();
    }

    return success;
}

- (void)loadDemoContent {
    if (!song_) return;

    // Mode 0: Song/Pattern Sequencer - Default 4-pattern loop
    // Steps 0-3: Pattern 1, Steps 4-7: Pattern 2, Steps 8-11: Pattern 3, Steps 12-15: Pattern 4
    // Only first 4 buttons lit, rest unlit
    Mode& mode0 = song_->getMode(0);
    Pattern& song_pattern = mode0.getPattern(0);

    // First 4 steps active (buttons 1-4 lit)
    for (int step = 0; step < 4; step++) {
        Event& event = song_pattern.getEvent(0, step);
        event.setSwitch(true);
        // S1 maps 0-127 to patterns 1-32, so pattern N = S1 value of (N-1) * 4
        // Pattern 1 = S1:0, Pattern 2 = S1:4, Pattern 3 = S1:8, Pattern 4 = S1:12
        event.setPot(0, step * 4);  // S1: 0, 4, 8, 12 -> Patterns 1, 2, 3, 4
    }

    // Remaining 12 steps inactive (buttons 5-16 unlit)
    for (int step = 4; step < 16; step++) {
        Event& event = song_pattern.getEvent(0, step);
        event.setSwitch(false);  // Button off
        event.setPot(0, 0);
    }

    // Mode 1: Drums
    Mode& mode1 = song_->getMode(1);
    Pattern& drum_pattern = mode1.getPattern(0);

    // Kick pattern (track 0) - on steps 0, 4, 8, 12
    int kick_steps[] = {0, 4, 8, 12};
    for (int step : kick_steps) {
        Event& event = drum_pattern.getEvent(0, step);
        event.setSwitch(true);
        event.setPot(0, 100);  // Velocity
        event.setPot(1, 50);   // Length
    }

    // Snare pattern (track 1) - on steps 4, 12
    int snare_steps[] = {4, 12};
    for (int step : snare_steps) {
        Event& event = drum_pattern.getEvent(1, step);
        event.setSwitch(true);
        event.setPot(0, 90);   // Velocity
        event.setPot(1, 30);   // Length
    }

    // Hi-hat pattern (track 2) - every other step
    for (int step = 0; step < 16; step += 2) {
        Event& event = drum_pattern.getEvent(2, step);
        event.setSwitch(true);
        event.setPot(0, 70);   // Velocity
        event.setPot(1, 20);   // Length
    }

    // Mode 2: Acid Bassline
    Mode& mode2 = song_->getMode(2);
    Pattern& acid_pattern = mode2.getPattern(0);

    // Step 0: Root (C2)
    acid_pattern.getEvent(0, 0).setSwitch(true);
    acid_pattern.getEvent(0, 0).setPot(0, 42);   // Pitch
    acid_pattern.getEvent(0, 0).setPot(1, 40);   // Length
    acid_pattern.getEvent(0, 0).setPot(2, 10);   // Slide
    acid_pattern.getEvent(0, 0).setPot(3, 60);   // Filter

    // Step 3: Fifth (G2)
    acid_pattern.getEvent(0, 3).setSwitch(true);
    acid_pattern.getEvent(0, 3).setPot(0, 67);
    acid_pattern.getEvent(0, 3).setPot(1, 35);
    acid_pattern.getEvent(0, 3).setPot(2, 60);
    acid_pattern.getEvent(0, 3).setPot(3, 80);

    // Step 4: Octave up (C3)
    acid_pattern.getEvent(0, 4).setSwitch(true);
    acid_pattern.getEvent(0, 4).setPot(0, 84);
    acid_pattern.getEvent(0, 4).setPot(1, 30);
    acid_pattern.getEvent(0, 4).setPot(2, 100);
    acid_pattern.getEvent(0, 4).setPot(3, 110);

    // Step 6: Fourth (F2)
    acid_pattern.getEvent(0, 6).setSwitch(true);
    acid_pattern.getEvent(0, 6).setPot(0, 59);
    acid_pattern.getEvent(0, 6).setPot(1, 40);
    acid_pattern.getEvent(0, 6).setPot(2, 20);
    acid_pattern.getEvent(0, 6).setPot(3, 70);

    // Step 8: Back to root (C2)
    acid_pattern.getEvent(0, 8).setSwitch(true);
    acid_pattern.getEvent(0, 8).setPot(0, 42);
    acid_pattern.getEvent(0, 8).setPot(1, 50);
    acid_pattern.getEvent(0, 8).setPot(2, 5);
    acid_pattern.getEvent(0, 8).setPot(3, 50);

    // Step 10: Minor third (Eb2)
    acid_pattern.getEvent(0, 10).setSwitch(true);
    acid_pattern.getEvent(0, 10).setPot(0, 50);
    acid_pattern.getEvent(0, 10).setPot(1, 35);
    acid_pattern.getEvent(0, 10).setPot(2, 40);
    acid_pattern.getEvent(0, 10).setPot(3, 65);

    if (hardware_) {
        hardware_->addLog("✓ Demo content loaded (Drums + Acid)");
    }
}

- (void)triggerLEDPattern:(NSString *)patternName {
    if (!engine_) return;

    std::string pattern = [patternName UTF8String];
    engine_->triggerLEDByName(pattern, 255);
}

- (BOOL)reloadMode:(NSInteger)mode {
    if (!modeLoader_ || !engine_) return NO;

    // Find modes directory (same logic as in initialize)
    NSString* modesPath = [[NSBundle mainBundle] pathForResource:@"modes" ofType:nil];
    if (!modesPath) {
        modesPath = [[NSBundle mainBundle] pathForResource:@"Resources/modes" ofType:nil];
    }

    if (!modesPath) {
        // For SPM builds, try relative to executable
        NSString* execPath = [[NSBundle mainBundle] executablePath];
        NSString* execDir = [execPath stringByDeletingLastPathComponent];

        modesPath = [[execDir stringByAppendingPathComponent:@"../../../../modes"] stringByStandardizingPath];
        if (![[NSFileManager defaultManager] fileExistsAtPath:modesPath]) {
            modesPath = [[execDir stringByAppendingPathComponent:@"../../../modes"] stringByStandardizingPath];
            if (![[NSFileManager defaultManager] fileExistsAtPath:modesPath]) {
                modesPath = [[execDir stringByAppendingPathComponent:@"../../modes"] stringByStandardizingPath];
                if (![[NSFileManager defaultManager] fileExistsAtPath:modesPath]) {
                    modesPath = [[execDir stringByAppendingPathComponent:@"../modes"] stringByStandardizingPath];
                    if (![[NSFileManager defaultManager] fileExistsAtPath:modesPath]) {
                        modesPath = nil;
                    }
                }
            }
        }
    }

    if (!modesPath) {
        NSLog(@"Could not find modes directory for reload");
        return NO;
    }

    // Find the mode file
    NSString* modeFilename = [NSString stringWithFormat:@"%02ld_*.lua", (long)mode];
    NSArray* files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:modesPath error:nil];
    NSString* modeFile = nil;

    for (NSString* file in files) {
        if ([file hasPrefix:[NSString stringWithFormat:@"%02ld_", (long)mode]] && [file hasSuffix:@".lua"]) {
            modeFile = [modesPath stringByAppendingPathComponent:file];
            break;
        }
    }

    if (!modeFile) {
        NSLog(@"Could not find mode file for mode %ld", (long)mode);
        return NO;
    }

    // Reload the mode
    std::string filepath = [modeFile UTF8String];
    int tempo = engine_->getTempo();
    bool success = modeLoader_->loadMode((int)mode, filepath, tempo);

    if (success) {
        NSLog(@"Reloaded mode %ld from %@", (long)mode, modeFile);
        std::cout << "Mode " << mode << " reloaded successfully" << std::endl;
    } else {
        NSLog(@"Failed to reload mode %ld", (long)mode);
    }

    return success ? YES : NO;
}

- (BOOL)validateLuaScript:(NSString *)path outError:(NSString **)outError {
    if (!path) {
        if (outError) {
            *outError = @"Invalid path";
        }
        return NO;
    }

    // Create a temporary Lua context to test validation
    gruvbok::LuaContext tempContext;
    bool success = tempContext.loadScript([path UTF8String]);

    if (!success) {
        if (outError) {
            std::string errorMsg = tempContext.getError();
            *outError = [NSString stringWithUTF8String:errorMsg.c_str()];
        }
        return NO;
    }

    // Check for required functions
    // Note: We could also call init() here, but that might have side effects
    // For now, loadScript() checks for init and process_event existence

    if (outError) {
        *outError = nil;
    }
    return YES;
}

- (void)setModeProgram:(NSInteger)mode program:(uint8_t)program {
    if (!engine_) return;
    engine_->setModeProgram((int)mode, program);
}

- (uint8_t)getModeProgram:(NSInteger)mode {
    if (!engine_) return 0;
    return engine_->getModeProgram((int)mode);
}

- (void)setEventPot:(NSInteger)mode
            pattern:(NSInteger)pattern
              track:(NSInteger)track
               step:(NSInteger)step
                pot:(NSInteger)pot
              value:(uint8_t)value {
    if (!engine_) return;
    engine_->setEventPot((int)mode, (int)pattern, (int)track, (int)step, (int)pot, value);
}

@end
