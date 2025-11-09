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
#include <SD.h>

using namespace gruvbok;

// Global instances - using static storage to avoid heap allocation
// Note: These are never destroyed (intentional for embedded firmware that runs forever)
static TeensyHardware hardware_instance;
static Song song_instance;
static ModeLoader mode_loader_instance;

// Pointers for compatibility with Engine API
TeensyHardware* hardware = &hardware_instance;
Song* song = &song_instance;
ModeLoader* mode_loader = &mode_loader_instance;
Engine* engine = nullptr;  // Still heap-allocated due to size, but properly managed

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    delay(100);  // Give serial time to initialize

    Serial.println("========================================");
    Serial.println("GRUVBOK - Teensy 4.1 Firmware");
    Serial.println("========================================");

    // Initialize hardware
    Serial.println("Initializing hardware...");
    if (!hardware->init()) {
        Serial.println("ERROR: Hardware initialization failed!");
        while (1) {
            delay(1000);  // Halt
        }
    }
    Serial.println("Hardware initialized successfully");

    // Song is already constructed via static storage
    Serial.println("Song initialized");

    // Initialize SD card
    Serial.println("Initializing SD card...");
    if (!SD.begin(BUILTIN_SDCARD)) {
        Serial.println("WARNING: SD card initialization failed!");
        Serial.println("         Insert microSD card with /modes directory");
        Serial.println("         Continuing without Lua modes (no MIDI output)");
    } else {
        Serial.println("SD card initialized successfully");

        // Load Lua modes from SD card
        Serial.println("Loading Lua modes from SD:/modes/...");
        int loaded_count = mode_loader->loadModesFromDirectory("/modes", 120);

        if (loaded_count > 0) {
            Serial.print("Successfully loaded ");
            Serial.print(loaded_count);
            Serial.println(" Lua modes");
        } else {
            Serial.println("WARNING: No Lua modes found on SD card");
            Serial.println("         Create SD:/modes/ directory with .lua files");
            Serial.println("         Example: 01_drums.lua, 02_acid.lua, etc.");
        }
    }

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
