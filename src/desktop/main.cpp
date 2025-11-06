#include "../core/song.h"
#include "../core/engine.h"
#include "../lua_bridge/mode_loader.h"
#include "desktop_hardware.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

using namespace gruvbok;

std::atomic<bool> running(true);

void signalHandler(int signal) {
    std::cout << "\nShutting down..." << std::endl;
    running = false;
}

void printHelp() {
    std::cout << "\n=== GRUVBOK Desktop ===" << std::endl;
    std::cout << "\nCommands:" << std::endl;
    std::cout << "  Space   - Start/Stop playback" << std::endl;
    std::cout << "  1-8     - Toggle step 1-8 for current track" << std::endl;
    std::cout << "  q/a     - Tempo up/down" << std::endl;
    std::cout << "  w/s     - Pattern up/down" << std::endl;
    std::cout << "  e/d     - Track up/down" << std::endl;
    std::cout << "  r/f     - Mode up/down" << std::endl;
    std::cout << "  h       - Show this help" << std::endl;
    std::cout << "  Ctrl+C  - Quit" << std::endl;
    std::cout << std::endl;
}

void printStatus(const Engine& engine) {
    std::cout << "\r[Mode:" << engine.getCurrentMode()
              << " Pat:" << engine.getCurrentPattern()
              << " Trk:" << engine.getCurrentTrack()
              << " Step:" << engine.getCurrentStep()
              << " Tempo:" << engine.getTempo() << "bpm"
              << " " << (engine.isPlaying() ? "PLAYING" : "STOPPED") << "] "
              << std::flush;
}

int main(int argc, char* argv[]) {
    std::cout << "=== GRUVBOK Desktop ===" << std::endl;
    std::cout << "Initializing..." << std::endl;

    // Set up signal handler
    std::signal(SIGINT, signalHandler);

    // Create hardware
    auto hardware = std::make_unique<DesktopHardware>();
    if (!hardware->init()) {
        std::cerr << "Failed to initialize hardware" << std::endl;
        return 1;
    }

    // Create song
    auto song = std::make_unique<Song>();

    // Load modes
    auto mode_loader = std::make_unique<ModeLoader>();
    int loaded = mode_loader->loadModesFromDirectory("modes", 120);

    if (loaded == 0) {
        std::cerr << "Warning: No modes loaded from 'modes/' directory" << std::endl;
        std::cerr << "Make sure you run from the project root directory" << std::endl;
    }

    // Create engine
    auto engine = std::make_unique<Engine>(song.get(), hardware.get(), mode_loader.get());

    // Set some default pattern data for testing
    std::cout << "\nCreating test pattern..." << std::endl;
    Mode& mode1 = song->getMode(1);  // Drum mode
    Pattern& pattern0 = mode1.getPattern(0);

    // Create a simple kick pattern on track 0 (steps 0, 4, 8, 12)
    for (int step : {0, 4, 8, 12}) {
        Event& event = pattern0.getEvent(0, step);
        event.setSwitch(true);
        event.setPot(0, 100);  // Velocity
        event.setPot(1, 50);   // Note length
    }

    // Create a snare pattern on track 1 (steps 4, 12)
    for (int step : {4, 12}) {
        Event& event = pattern0.getEvent(1, step);
        event.setSwitch(true);
        event.setPot(0, 90);
        event.setPot(1, 30);
    }

    // Create a hi-hat pattern on track 2 (every other step)
    for (int step = 0; step < 16; step += 2) {
        Event& event = pattern0.getEvent(2, step);
        event.setSwitch(true);
        event.setPot(0, 70);
        event.setPot(1, 20);
    }

    printHelp();

    // Start playback
    engine->start();

    std::cout << "\nRunning main loop (press Ctrl+C to quit)..." << std::endl;

    // Simple keyboard input simulation (for demo purposes)
    // In a real implementation, this would use SDL2 or ncurses
    int frame_count = 0;

    while (running) {
        // Update engine
        engine->update();

        // Print status every 10 frames
        if (frame_count++ % 10 == 0) {
            printStatus(*engine);
        }

        // Sleep to avoid spinning
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "\nCleaning up..." << std::endl;
    engine->stop();
    hardware->shutdown();

    std::cout << "Goodbye!" << std::endl;
    return 0;
}
