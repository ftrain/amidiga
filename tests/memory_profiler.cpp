/**
 * Memory Profiler for GRUVBOK
 *
 * Analyzes memory usage for Teensy 4.1 deployment (1MB RAM limit).
 *
 * This tool:
 * 1. Calculates static memory usage (sizeof all major structures)
 * 2. Simulates runtime memory allocation
 * 3. Reports total memory footprint
 * 4. Identifies memory hotspots
 *
 * Usage:
 *   ./memory_profiler
 *
 * Expected output:
 *   - Total static memory
 *   - Total dynamic memory
 *   - Memory headroom for Teensy 4.1
 */

#include "../src/core/song.h"
#include "../src/core/event.h"
#include "../src/core/pattern.h"
#include "../src/core/engine.h"
#include "../src/core/led_controller.h"
#include "../src/core/midi_clock_manager.h"
#include "../src/core/mode0_sequencer.h"
#include "../src/core/playback_state.h"
#include "../src/hardware/midi_scheduler.h"
#include "../src/lua_bridge/lua_context.h"
#include <iostream>
#include <iomanip>
#include <vector>

using namespace gruvbok;

// ANSI color codes for terminal output
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"

// Teensy 4.1 specifications
constexpr size_t TEENSY_RAM_TOTAL = 1024 * 1024;  // 1 MB
constexpr size_t TEENSY_RAM_SAFE_LIMIT = 900 * 1024;  // Leave 124KB for safety margin

// Helper to print memory size in human-readable format
std::string formatBytes(size_t bytes) {
    if (bytes < 1024) {
        return std::to_string(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        return std::to_string(bytes / 1024) + " KB (" + std::to_string(bytes) + " bytes)";
    } else {
        double mb = static_cast<double>(bytes) / (1024 * 1024);
        return std::to_string(static_cast<int>(mb * 100) / 100.0) + " MB (" + std::to_string(bytes) + " bytes)";
    }
}

// Helper to print a memory category
void printCategory(const std::string& name, size_t bytes, size_t total) {
    double percentage = (static_cast<double>(bytes) / total) * 100.0;
    std::cout << "  " << COLOR_CYAN << std::left << std::setw(40) << name << COLOR_RESET
              << std::right << std::setw(12) << formatBytes(bytes)
              << "  (" << std::fixed << std::setprecision(1) << percentage << "%)"
              << std::endl;
}

// Print memory analysis
void analyzeMemory() {
    std::cout << COLOR_BOLD << "\n=== GRUVBOK Memory Profiler ===" << COLOR_RESET << std::endl;
    std::cout << "Target platform: Teensy 4.1 (1 MB RAM)\n" << std::endl;

    // ========================================================================
    // Static Memory Analysis
    // ========================================================================

    std::cout << COLOR_BOLD << "STATIC MEMORY (sizeof types):" << COLOR_RESET << std::endl;

    size_t total_static = 0;

    // Core data structures
    size_t event_size = sizeof(Event);
    size_t track_size = sizeof(Track);
    size_t pattern_size = sizeof(Pattern);
    size_t mode_size = sizeof(Mode);
    size_t song_size = sizeof(Song);

    printCategory("Event (1 bit switch + 4×7bit pots)", event_size, TEENSY_RAM_TOTAL);
    printCategory("Track (16 events)", track_size, TEENSY_RAM_TOTAL);
    printCategory("Pattern (8 tracks)", pattern_size, TEENSY_RAM_TOTAL);
    printCategory("Mode (32 patterns)", mode_size, TEENSY_RAM_TOTAL);
    printCategory("Song (15 modes)", song_size, TEENSY_RAM_TOTAL);
    total_static += song_size;

    // Engine and components
    size_t engine_size = sizeof(Engine);
    size_t led_controller_size = sizeof(LEDController);
    size_t clock_manager_size = sizeof(MidiClockManager);
    size_t mode0_sequencer_size = sizeof(Mode0Sequencer);
    size_t playback_state_size = sizeof(PlaybackState);
    size_t midi_scheduler_size = sizeof(MidiScheduler);

    std::cout << std::endl;
    printCategory("Engine (coordinator)", engine_size, TEENSY_RAM_TOTAL);
    printCategory("LEDController", led_controller_size, TEENSY_RAM_TOTAL);
    printCategory("MidiClockManager", clock_manager_size, TEENSY_RAM_TOTAL);
    printCategory("Mode0Sequencer", mode0_sequencer_size, TEENSY_RAM_TOTAL);
    printCategory("PlaybackState", playback_state_size, TEENSY_RAM_TOTAL);
    printCategory("MidiScheduler (64 event buffer)", midi_scheduler_size, TEENSY_RAM_TOTAL);

    total_static += engine_size;
    total_static += led_controller_size;
    total_static += clock_manager_size;
    total_static += mode0_sequencer_size;
    total_static += playback_state_size;
    total_static += midi_scheduler_size;

    // Lua contexts (15 modes)
    size_t lua_context_size = sizeof(LuaContext);
    size_t lua_contexts_total = lua_context_size * Song::NUM_MODES;

    std::cout << std::endl;
    printCategory("LuaContext (per mode)", lua_context_size, TEENSY_RAM_TOTAL);
    printCategory("LuaContext × 15 modes", lua_contexts_total, TEENSY_RAM_TOTAL);
    total_static += lua_contexts_total;

    std::cout << std::endl;
    std::cout << COLOR_BOLD << "TOTAL STATIC MEMORY: " << COLOR_GREEN
              << formatBytes(total_static) << COLOR_RESET << std::endl;

    // ========================================================================
    // Dynamic Memory Estimation
    // ========================================================================

    std::cout << "\n" << COLOR_BOLD << "ESTIMATED DYNAMIC MEMORY:" << COLOR_RESET << std::endl;

    size_t total_dynamic = 0;

    // Lua VM overhead (estimated ~50KB per state with compiled scripts)
    size_t lua_vm_overhead = 50 * 1024 * Song::NUM_MODES;
    printCategory("Lua VM states (15 × 50KB est.)", lua_vm_overhead, TEENSY_RAM_TOTAL);
    total_dynamic += lua_vm_overhead;

    // Stack estimation (Teensy default: 8KB main + 4KB per thread)
    size_t stack_estimate = 12 * 1024;
    printCategory("Stack (main + threads, est.)", stack_estimate, TEENSY_RAM_TOTAL);
    total_dynamic += stack_estimate;

    // Heap fragmentation buffer (10% safety margin)
    size_t fragmentation_buffer = (total_static + total_dynamic) / 10;
    printCategory("Heap fragmentation buffer (10%)", fragmentation_buffer, TEENSY_RAM_TOTAL);
    total_dynamic += fragmentation_buffer;

    std::cout << std::endl;
    std::cout << COLOR_BOLD << "TOTAL DYNAMIC MEMORY: " << COLOR_YELLOW
              << formatBytes(total_dynamic) << COLOR_RESET << std::endl;

    // ========================================================================
    // Total Memory Usage
    // ========================================================================

    size_t total_memory = total_static + total_dynamic;
    size_t remaining = TEENSY_RAM_TOTAL - total_memory;
    double usage_percent = (static_cast<double>(total_memory) / TEENSY_RAM_TOTAL) * 100.0;

    std::cout << "\n" << COLOR_BOLD << "=== TOTAL MEMORY USAGE ===" << COLOR_RESET << std::endl;
    printCategory("Static memory", total_static, TEENSY_RAM_TOTAL);
    printCategory("Dynamic memory (estimated)", total_dynamic, TEENSY_RAM_TOTAL);

    std::cout << std::endl;
    std::cout << COLOR_BOLD << "TOTAL: ";

    if (total_memory > TEENSY_RAM_SAFE_LIMIT) {
        std::cout << COLOR_RED << formatBytes(total_memory);
    } else if (total_memory > TEENSY_RAM_SAFE_LIMIT * 0.8) {
        std::cout << COLOR_YELLOW << formatBytes(total_memory);
    } else {
        std::cout << COLOR_GREEN << formatBytes(total_memory);
    }

    std::cout << COLOR_RESET << " (" << std::fixed << std::setprecision(1)
              << usage_percent << "% of 1 MB)" << std::endl;

    std::cout << COLOR_BOLD << "REMAINING: ";
    if (remaining < 100 * 1024) {
        std::cout << COLOR_RED;
    } else if (remaining < 200 * 1024) {
        std::cout << COLOR_YELLOW;
    } else {
        std::cout << COLOR_GREEN;
    }
    std::cout << formatBytes(remaining) << COLOR_RESET << std::endl;

    // ========================================================================
    // Memory Hotspots & Recommendations
    // ========================================================================

    std::cout << "\n" << COLOR_BOLD << "=== MEMORY HOTSPOTS ===" << COLOR_RESET << std::endl;

    struct MemoryHotspot {
        std::string name;
        size_t bytes;
    };

    std::vector<MemoryHotspot> hotspots = {
        {"Song data structure", song_size},
        {"Lua VM states (15 modes)", lua_vm_overhead},
        {"MIDI Scheduler event buffer", midi_scheduler_size},
        {"Engine + components", engine_size + led_controller_size + clock_manager_size +
                                mode0_sequencer_size + playback_state_size}
    };

    // Sort by size (descending)
    std::sort(hotspots.begin(), hotspots.end(),
              [](const MemoryHotspot& a, const MemoryHotspot& b) {
                  return a.bytes > b.bytes;
              });

    for (size_t i = 0; i < hotspots.size(); ++i) {
        std::cout << "  " << (i + 1) << ". " << std::left << std::setw(40)
                  << hotspots[i].name << formatBytes(hotspots[i].bytes) << std::endl;
    }

    // ========================================================================
    // Safety Analysis
    // ========================================================================

    std::cout << "\n" << COLOR_BOLD << "=== SAFETY ANALYSIS ===" << COLOR_RESET << std::endl;

    if (total_memory > TEENSY_RAM_TOTAL) {
        std::cout << "  " << COLOR_RED << "⚠ CRITICAL: Memory usage exceeds Teensy RAM!"
                  << COLOR_RESET << std::endl;
        std::cout << "  Overrun: " << formatBytes(total_memory - TEENSY_RAM_TOTAL) << std::endl;
    } else if (total_memory > TEENSY_RAM_SAFE_LIMIT) {
        std::cout << "  " << COLOR_YELLOW << "⚠ WARNING: Memory usage exceeds safe limit (900 KB)"
                  << COLOR_RESET << std::endl;
        std::cout << "  Recommend leaving at least 100 KB headroom for runtime allocation." << std::endl;
    } else if (total_memory > TEENSY_RAM_SAFE_LIMIT * 0.8) {
        std::cout << "  " << COLOR_YELLOW << "✓ ACCEPTABLE: Memory usage within limits"
                  << COLOR_RESET << std::endl;
        std::cout << "  Headroom: " << formatBytes(remaining) << " remaining." << std::endl;
    } else {
        std::cout << "  " << COLOR_GREEN << "✓ EXCELLENT: Plenty of memory headroom!"
                  << COLOR_RESET << std::endl;
        std::cout << "  Headroom: " << formatBytes(remaining) << " remaining." << std::endl;
    }

    // ========================================================================
    // Optimization Recommendations
    // ========================================================================

    std::cout << "\n" << COLOR_BOLD << "=== OPTIMIZATION RECOMMENDATIONS ===" << COLOR_RESET << std::endl;

    if (song_size > 300 * 1024) {
        std::cout << "  • Song data is large (" << formatBytes(song_size) << ")." << std::endl;
        std::cout << "    Consider reducing NUM_PATTERNS or NUM_MODES if not needed." << std::endl;
    }

    if (lua_vm_overhead > 800 * 1024) {
        std::cout << "  • Lua VM states consume significant memory (" << formatBytes(lua_vm_overhead) << ")." << std::endl;
        std::cout << "    Consider compiling modes to bytecode or reducing Lua GC limits." << std::endl;
    }

    if (total_memory < TEENSY_RAM_SAFE_LIMIT * 0.5) {
        std::cout << "  • " << COLOR_GREEN << "Excellent memory efficiency!"
                  << COLOR_RESET << " (" << formatBytes(remaining) << " remaining)" << std::endl;
        std::cout << "    You could add more features or increase buffer sizes if needed." << std::endl;
    }

    std::cout << std::endl;
}

int main() {
    analyzeMemory();
    return 0;
}
