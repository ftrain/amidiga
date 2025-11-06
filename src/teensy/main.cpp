/**
 * GRUVBOK - Teensy 4.1 Main Entry Point
 *
 * This is the main firmware for the GRUVBOK hardware groovebox.
 * It initializes the hardware, loads Lua modes, and runs the playback engine.
 */

#include "teensy_hardware.h"
#include "../core/song.h"
#include "../core/engine.h"
#include "../lua_bridge/mode_loader.h"

using namespace gruvbok;

// Global instances
Song* song = nullptr;
TeensyHardware* hardware = nullptr;
ModeLoader* mode_loader = nullptr;
Engine* engine = nullptr;

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    delay(100);  // Give serial time to initialize

    Serial.println("========================================");
    Serial.println("GRUVBOK - Teensy 4.1 Firmware");
    Serial.println("========================================");

    // Initialize hardware
    Serial.println("Initializing hardware...");
    hardware = new TeensyHardware();
    if (!hardware->init()) {
        Serial.println("ERROR: Hardware initialization failed!");
        while (1) {
            delay(1000);  // Halt
        }
    }
    Serial.println("Hardware initialized successfully");

    // Create song data structure
    Serial.println("Creating song...");
    song = new Song();
    Serial.println("Song created");

    // Load Lua modes
    Serial.println("Loading Lua modes...");
    mode_loader = new ModeLoader();

    // For now, we'll skip Lua loading on Teensy until SD card is set up
    // TODO: Implement SD card mode loading from /modes directory
    Serial.println("WARNING: Lua mode loading not yet implemented for Teensy");
    Serial.println("         Using placeholder modes (will output no MIDI)");
    Serial.println("         To enable: Add SD card support and call mode_loader->loadModesFromDirectory(\"/modes\", 120)");

    // Create and start engine
    Serial.println("Creating engine...");
    engine = new Engine(song, hardware, mode_loader);
    engine->setTempo(120);  // Default tempo
    Serial.println("Engine created");

    // Start playback
    Serial.println("Starting playback...");
    engine->start();
    Serial.println("Playback started");

    Serial.println("========================================");
    Serial.println("GRUVBOK is ready!");
    Serial.println("Tempo: 120 BPM");
    Serial.println("Mode: 1 (Drums)");
    Serial.println("Pattern: 0");
    Serial.println("Track: 0");
    Serial.println("========================================");
}

void loop() {
    // Main update loop
    engine->update();

    // Optional: Print status periodically
    static uint32_t last_status_print = 0;
    uint32_t current_time = millis();

    if (current_time - last_status_print >= 5000) {  // Every 5 seconds
        last_status_print = current_time;

        Serial.print("Status: ");
        Serial.print("Mode=");
        Serial.print(engine->getCurrentMode());
        Serial.print(" Pattern=");
        Serial.print(engine->getCurrentPattern());
        Serial.print(" Track=");
        Serial.print(engine->getCurrentTrack());
        Serial.print(" Step=");
        Serial.print(engine->getCurrentStep());
        Serial.print(" Tempo=");
        Serial.print(engine->getTempo());
        Serial.println(" BPM");
    }

    // Small delay to avoid hogging CPU (though Teensy is fast enough)
    // delayMicroseconds(10);  // Optional: very small delay
}

/**
 * Note: To complete the Teensy port, you still need to:
 *
 * 1. Add Lua library compilation:
 *    - Copy Lua 5.4 source to lib/lua/
 *    - Configure PlatformIO to build it
 *
 * 2. Add SD card support:
 *    - Include SD.h and SdFat library
 *    - Mount SD card in setup()
 *    - Modify ModeLoader to read .lua files from SD
 *
 * 3. Test on real hardware:
 *    - Connect buttons, pots, and verify pin mappings
 *    - Test MIDI output
 *    - Verify LED tempo indicator
 *    - Test memory usage (should be <600KB)
 *
 * 4. Optimize:
 *    - Profile with ARM Serial Wire Debug
 *    - Optimize Lua GC settings for real-time
 *    - Test with all 15 modes loaded
 */
