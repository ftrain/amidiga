#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * Event data structure for SwiftUI
 */
@interface EventData : NSObject
@property (nonatomic, assign) NSInteger step;
@property (nonatomic, assign) BOOL switchOn;
@property (nonatomic, strong) NSArray<NSNumber *> *pots;  // 4 pot values (0-127)

- (instancetype)initWithStep:(NSInteger)step
                    switchOn:(BOOL)switchOn
                        pots:(NSArray<NSNumber *> *)pots;
@end

/**
 * Objective-C++ wrapper for the C++ Engine
 * Bridges C++ to Swift via Objective-C
 */
@interface EngineWrapper : NSObject

// Lifecycle
- (instancetype)initWithPlatform:(NSString *)platform;  // "macOS" or "iOS"
- (BOOL)initialize;
- (void)start;
- (void)stop;
- (void)update;  // Call at ~60fps from SwiftUI

// State
- (BOOL)isPlaying;
- (BOOL)isDirty;
- (NSInteger)getCurrentMode;
- (NSInteger)getCurrentPattern;
- (NSInteger)getCurrentTrack;
- (NSInteger)getCurrentStep;
- (NSInteger)getSongModeStep;  // Mode 0 step (1/16th speed)
- (NSInteger)getTempo;
- (BOOL)getLEDState;

// Control
- (void)setTempo:(NSInteger)tempo;
- (void)setMode:(NSInteger)mode;
- (void)setPattern:(NSInteger)pattern;
- (void)setTrack:(NSInteger)track;

// Input simulation (from SwiftUI)
- (void)simulateButton:(NSInteger)button pressed:(BOOL)pressed;
- (void)simulateRotaryPot:(NSInteger)pot value:(uint8_t)value;
- (void)simulateSliderPot:(NSInteger)pot value:(uint8_t)value;

// Data access
- (NSArray<EventData *> *)getTrackEvents;  // Returns 16 events for current track
- (NSArray<NSString *> *)getLogMessages;
- (void)clearLog;

// Audio control
- (BOOL)initAudioWithSoundFont:(nullable NSString *)soundfontPath;
- (void)setUseInternalAudio:(BOOL)useInternal;
- (void)setUseExternalMIDI:(BOOL)useExternal;
- (BOOL)isAudioReady;
- (void)setAudioGain:(float)gain;

// MIDI output port management
- (NSInteger)getMidiOutputCount;
- (NSString *)getMidiOutputName:(NSInteger)index;
- (BOOL)selectMidiOutput:(NSInteger)index;
- (NSInteger)getCurrentMidiOutput;

// MIDI input port management (mirror mode)
- (NSInteger)getMidiInputCount;
- (NSString *)getMidiInputName:(NSInteger)index;
- (BOOL)selectMidiInput:(NSInteger)index;
- (NSInteger)getCurrentMidiInput;
- (BOOL)isMirrorModeEnabled;
- (void)setMirrorMode:(BOOL)enabled;

// Song persistence
- (BOOL)saveSongToPath:(NSString *)path name:(NSString *)name;
- (BOOL)loadSongFromPath:(NSString *)path
               outName:(NSString *_Nullable *_Nullable)outName
             outTempo:(NSInteger *)outTempo;

// Demo content
- (void)loadDemoContent;

// LED pattern control
- (void)triggerLEDPattern:(NSString *)patternName;

// Lua mode management
- (BOOL)reloadMode:(NSInteger)mode;
- (BOOL)validateLuaScript:(NSString *)path outError:(NSString *_Nullable *_Nullable)outError;

@end

NS_ASSUME_NONNULL_END
