#import "EngineWrapper.h"
#include "../../../../src/core/song.h"
#include "../../../../src/core/engine.h"
#include "../../../../src/lua_bridge/mode_loader.h"

#if TARGET_OS_IPHONE
#include "../../iOS/IOSHardware.h"
#else
#include "../../macOS/MacOSHardware.h"
#endif

#include <memory>
#include <vector>
#include <string>

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

    // Load Lua modes from bundle
    NSString* modesPath = [[NSBundle mainBundle] pathForResource:@"modes" ofType:nil];
    if (!modesPath) {
        // Try Resources/modes for app bundle
        modesPath = [[NSBundle mainBundle] pathForResource:@"Resources/modes" ofType:nil];
    }

    if (modesPath) {
        int loaded = modeLoader_->loadModesFromDirectory([modesPath UTF8String], 120);
        NSLog(@"Loaded %d Lua modes from: %@", loaded, modesPath);
    } else {
        NSLog(@"WARNING: modes/ directory not found in bundle");
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
    engine_->start();

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

@end
