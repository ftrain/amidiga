/**
 * Unit tests for Mode and Song classes
 *
 * Tests the top-level data structure hierarchy:
 * - Mode: Contains 32 Patterns (one per mode)
 * - Song: Contains 15 Modes (one per MIDI channel)
 */

#include "../src/core/song.h"
#include <iostream>
#include <cassert>
#include <fstream>

// Simple test framework (same as other tests)
int test_count = 0;
int pass_count = 0;
int fail_count = 0;

#define TEST(name) \
    void test_##name(); \
    void run_test_##name() { \
        std::cout << "Running test: " << #name << "... "; \
        try { \
            test_##name(); \
            std::cout << "PASS" << std::endl; \
            pass_count++; \
        } catch (const std::exception& e) { \
            std::cout << "FAIL: " << e.what() << std::endl; \
            fail_count++; \
        } \
        test_count++; \
    } \
    void test_##name()

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        throw std::runtime_error(std::string("Expected ") + #a + " == " + #b + \
                                 ", got " + std::to_string(a) + " != " + std::to_string(b)); \
    }

#define ASSERT_TRUE(expr) \
    if (!(expr)) { \
        throw std::runtime_error(std::string("Expected ") + #expr + " to be true"); \
    }

#define ASSERT_FALSE(expr) \
    if ((expr)) { \
        throw std::runtime_error(std::string("Expected ") + #expr + " to be false"); \
    }

using namespace gruvbok;

// ============================================================================
// Mode Tests
// ============================================================================

TEST(mode_default_constructor) {
    Mode mode;

    // All patterns should be empty
    for (int pattern_num = 0; pattern_num < Mode::NUM_PATTERNS; pattern_num++) {
        const Pattern& pattern = mode.getPattern(pattern_num);
        for (int track = 0; track < Pattern::NUM_TRACKS; track++) {
            for (int step = 0; step < Track::NUM_EVENTS; step++) {
                const Event& evt = pattern.getEvent(track, step);
                ASSERT_FALSE(evt.getSwitch());
            }
        }
    }
}

TEST(mode_set_get_pattern) {
    Mode mode;

    Pattern pattern;
    // Create a unique pattern
    for (int track = 0; track < Pattern::NUM_TRACKS; track++) {
        Event evt;
        evt.setSwitch(true);
        evt.setPot(0, track * 10);
        pattern.setEvent(track, 0, evt);
    }

    mode.setPattern(5, pattern);

    // Retrieve and verify
    const Pattern& retrieved = mode.getPattern(5);
    for (int track = 0; track < Pattern::NUM_TRACKS; track++) {
        const Event& evt = retrieved.getEvent(track, 0);
        ASSERT_TRUE(evt.getSwitch());
        ASSERT_EQ(evt.getPot(0), track * 10);
    }

    // Other patterns should remain empty
    const Pattern& pattern0 = mode.getPattern(0);
    const Event& evt0 = pattern0.getEvent(0, 0);
    ASSERT_FALSE(evt0.getSwitch());
}

TEST(mode_pattern_isolation) {
    // Test that patterns don't affect each other
    Mode mode;

    for (int pattern_num = 0; pattern_num < Mode::NUM_PATTERNS; pattern_num++) {
        Pattern& pattern = mode.getPattern(pattern_num);
        Event evt;
        evt.setSwitch(true);
        evt.setPot(0, pattern_num);
        pattern.setEvent(0, 0, evt);
    }

    // Verify each pattern
    for (int pattern_num = 0; pattern_num < Mode::NUM_PATTERNS; pattern_num++) {
        const Pattern& pattern = mode.getPattern(pattern_num);
        const Event& evt = pattern.getEvent(0, 0);
        ASSERT_TRUE(evt.getSwitch());
        ASSERT_EQ(evt.getPot(0), pattern_num);
    }
}

TEST(mode_clear) {
    Mode mode;

    // Set data in all patterns
    for (int pattern_num = 0; pattern_num < Mode::NUM_PATTERNS; pattern_num++) {
        Pattern& pattern = mode.getPattern(pattern_num);
        Event evt;
        evt.setSwitch(true);
        evt.setPot(0, 127);
        pattern.setEvent(0, 0, evt);
    }

    // Clear mode
    mode.clear();

    // All patterns should be empty
    for (int pattern_num = 0; pattern_num < Mode::NUM_PATTERNS; pattern_num++) {
        const Pattern& pattern = mode.getPattern(pattern_num);
        const Event& evt = pattern.getEvent(0, 0);
        ASSERT_FALSE(evt.getSwitch());
        ASSERT_EQ(evt.getPot(0), 0);
    }
}

TEST(mode_num_patterns_constant) {
    // Verify the constant matches the design (32 patterns)
    ASSERT_EQ(Mode::NUM_PATTERNS, 32);
}

// ============================================================================
// Song Tests
// ============================================================================

TEST(song_default_constructor) {
    Song song;

    // All modes should be empty
    for (int mode_num = 0; mode_num < Song::NUM_MODES; mode_num++) {
        const Mode& mode = song.getMode(mode_num);
        const Pattern& pattern = mode.getPattern(0);
        const Event& evt = pattern.getEvent(0, 0);
        ASSERT_FALSE(evt.getSwitch());
    }
}

TEST(song_set_get_mode) {
    Song song;

    Mode mode;
    // Create a unique mode
    Pattern& pattern = mode.getPattern(0);
    for (int track = 0; track < Pattern::NUM_TRACKS; track++) {
        Event evt;
        evt.setSwitch(true);
        evt.setPot(0, track * 15);
        pattern.setEvent(track, 0, evt);
    }

    song.setMode(7, mode);

    // Retrieve and verify
    const Mode& retrieved = song.getMode(7);
    const Pattern& retrieved_pattern = retrieved.getPattern(0);
    for (int track = 0; track < Pattern::NUM_TRACKS; track++) {
        const Event& evt = retrieved_pattern.getEvent(track, 0);
        ASSERT_TRUE(evt.getSwitch());
        ASSERT_EQ(evt.getPot(0), track * 15);
    }

    // Other modes should remain empty
    const Mode& mode0 = song.getMode(0);
    const Pattern& pattern0 = mode0.getPattern(0);
    const Event& evt0 = pattern0.getEvent(0, 0);
    ASSERT_FALSE(evt0.getSwitch());
}

TEST(song_mode_isolation) {
    // Test that modes don't affect each other
    Song song;

    for (int mode_num = 0; mode_num < Song::NUM_MODES; mode_num++) {
        Mode& mode = song.getMode(mode_num);
        Pattern& pattern = mode.getPattern(0);
        Event evt;
        evt.setSwitch(true);
        evt.setPot(0, mode_num * 8);
        pattern.setEvent(0, 0, evt);
    }

    // Verify each mode
    for (int mode_num = 0; mode_num < Song::NUM_MODES; mode_num++) {
        const Mode& mode = song.getMode(mode_num);
        const Pattern& pattern = mode.getPattern(0);
        const Event& evt = pattern.getEvent(0, 0);
        ASSERT_TRUE(evt.getSwitch());
        ASSERT_EQ(evt.getPot(0), mode_num * 8);
    }
}

TEST(song_clear) {
    Song song;

    // Set data in all modes
    for (int mode_num = 0; mode_num < Song::NUM_MODES; mode_num++) {
        Mode& mode = song.getMode(mode_num);
        Pattern& pattern = mode.getPattern(0);
        Event evt;
        evt.setSwitch(true);
        evt.setPot(0, 100);
        pattern.setEvent(0, 0, evt);
    }

    // Clear song
    song.clear();

    // All modes should be empty
    for (int mode_num = 0; mode_num < Song::NUM_MODES; mode_num++) {
        const Mode& mode = song.getMode(mode_num);
        const Pattern& pattern = mode.getPattern(0);
        const Event& evt = pattern.getEvent(0, 0);
        ASSERT_FALSE(evt.getSwitch());
        ASSERT_EQ(evt.getPot(0), 0);
    }
}

TEST(song_num_modes_constant) {
    // Verify the constant matches the design (15 modes, 0-14)
    ASSERT_EQ(Song::NUM_MODES, 15);
}

TEST(song_memory_footprint) {
    // Test that memory calculation is reasonable
    size_t footprint = Song::getMemoryFootprint();

    // Expected: 15 modes × 32 patterns × 8 tracks × 16 events × 4 bytes
    size_t expected = 15 * 32 * 8 * 16 * sizeof(uint32_t);
    ASSERT_EQ(footprint, expected);

    // Verify it's within Teensy 4.1 limits (1MB RAM)
    // Should be around 245 KB
    ASSERT_TRUE(footprint < 1024 * 1024);  // Less than 1 MB
    ASSERT_TRUE(footprint > 200 * 1024);   // More than 200 KB

    std::cout << " [" << footprint << " bytes] ";
}

TEST(song_full_hierarchy) {
    // Test the complete data hierarchy: Song → Mode → Pattern → Track → Event
    Song song;

    // Set data at different levels
    Mode& mode = song.getMode(3);
    Pattern& pattern = mode.getPattern(7);
    Event evt;
    evt.setSwitch(true);
    evt.setPot(0, 11);
    evt.setPot(1, 22);
    evt.setPot(2, 33);
    evt.setPot(3, 44);
    pattern.setEvent(5, 9, evt);

    // Retrieve through full hierarchy
    const Event& retrieved = song.getMode(3).getPattern(7).getEvent(5, 9);
    ASSERT_TRUE(retrieved.getSwitch());
    ASSERT_EQ(retrieved.getPot(0), 11);
    ASSERT_EQ(retrieved.getPot(1), 22);
    ASSERT_EQ(retrieved.getPot(2), 33);
    ASSERT_EQ(retrieved.getPot(3), 44);
}

// ============================================================================
// Save/Load Tests
// ============================================================================

TEST(song_save_empty) {
    // Test saving an empty song
    Song song;
    ASSERT_TRUE(song.save("/tmp/test_empty.json"));
}

TEST(song_save_load_roundtrip) {
    // Create a song with some events
    Song song1;

    // Mode 0, Pattern 0, Track 0
    Event evt1;
    evt1.setSwitch(true);
    evt1.setPot(0, 60);
    evt1.setPot(1, 100);
    evt1.setPot(2, 50);
    evt1.setPot(3, 80);
    song1.getMode(0).getPattern(0).setEvent(0, 5, evt1);

    // Mode 1, Pattern 2, Track 3, different step
    Event evt2;
    evt2.setSwitch(true);
    evt2.setPot(0, 127);
    evt2.setPot(1, 0);
    evt2.setPot(2, 64);
    evt2.setPot(3, 32);
    song1.getMode(1).getPattern(2).setEvent(3, 10, evt2);

    // Mode 14, Pattern 31 (boundaries)
    Event evt3;
    evt3.setSwitch(true);
    evt3.setPot(0, 1);
    evt3.setPot(1, 2);
    evt3.setPot(2, 3);
    evt3.setPot(3, 4);
    song1.getMode(14).getPattern(31).setEvent(7, 15, evt3);

    // Save to file
    const char* filepath = "/tmp/test_roundtrip.json";
    ASSERT_TRUE(song1.save(filepath));

    // Load into new song
    Song song2;
    ASSERT_TRUE(song2.load(filepath));

    // Verify first event
    const Event& loaded1 = song2.getMode(0).getPattern(0).getEvent(0, 5);
    ASSERT_TRUE(loaded1.getSwitch());
    ASSERT_EQ(loaded1.getPot(0), 60);
    ASSERT_EQ(loaded1.getPot(1), 100);
    ASSERT_EQ(loaded1.getPot(2), 50);
    ASSERT_EQ(loaded1.getPot(3), 80);

    // Verify second event
    const Event& loaded2 = song2.getMode(1).getPattern(2).getEvent(3, 10);
    ASSERT_TRUE(loaded2.getSwitch());
    ASSERT_EQ(loaded2.getPot(0), 127);
    ASSERT_EQ(loaded2.getPot(1), 0);
    ASSERT_EQ(loaded2.getPot(2), 64);
    ASSERT_EQ(loaded2.getPot(3), 32);

    // Verify third event (boundaries)
    const Event& loaded3 = song2.getMode(14).getPattern(31).getEvent(7, 15);
    ASSERT_TRUE(loaded3.getSwitch());
    ASSERT_EQ(loaded3.getPot(0), 1);
    ASSERT_EQ(loaded3.getPot(1), 2);
    ASSERT_EQ(loaded3.getPot(2), 3);
    ASSERT_EQ(loaded3.getPot(3), 4);

    // Verify other events are empty
    const Event& empty = song2.getMode(0).getPattern(0).getEvent(0, 0);
    ASSERT_FALSE(empty.getSwitch());
}

TEST(song_load_nonexistent) {
    // Test loading a file that doesn't exist
    Song song;
    ASSERT_FALSE(song.load("/tmp/nonexistent_file.json"));
}

TEST(song_load_invalid_json) {
    // Create an invalid JSON file
    std::ofstream file("/tmp/test_invalid.json");
    file << "{ this is not valid JSON }";
    file.close();

    Song song;
    ASSERT_FALSE(song.load("/tmp/test_invalid.json"));
}

TEST(song_load_wrong_version) {
    // Create a JSON file with wrong version
    std::ofstream file("/tmp/test_wrong_version.json");
    file << R"({
        "version": "2.0",
        "events": []
    })";
    file.close();

    Song song;
    ASSERT_FALSE(song.load("/tmp/test_wrong_version.json"));
}

TEST(song_sparse_format) {
    // Test that sparse format only saves active events
    Song song;

    // Add just a few events out of 61,440 possible
    Event evt;
    evt.setSwitch(true);
    evt.setPot(0, 99);
    song.getMode(0).getPattern(0).setEvent(0, 0, evt);
    song.getMode(5).getPattern(10).setEvent(3, 7, evt);

    const char* filepath = "/tmp/test_sparse.json";
    ASSERT_TRUE(song.save(filepath));

    // Read file and verify it's small (not full dump)
    std::ifstream file(filepath);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    // File should be small (< 1KB for 2 events)
    ASSERT_TRUE(content.size() < 1024);

    // Should contain exactly 2 events in the events array
    size_t event_count = 0;
    size_t pos = 0;
    while ((pos = content.find("\"mode\"", pos)) != std::string::npos) {
        event_count++;
        pos++;
    }
    ASSERT_EQ(event_count, 2u);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "GRUVBOK Mode/Song Tests" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;

    // Mode tests
    run_test_mode_default_constructor();
    run_test_mode_set_get_pattern();
    run_test_mode_pattern_isolation();
    run_test_mode_clear();
    run_test_mode_num_patterns_constant();

    // Song tests
    run_test_song_default_constructor();
    run_test_song_set_get_mode();
    run_test_song_mode_isolation();
    run_test_song_clear();
    run_test_song_num_modes_constant();
    run_test_song_memory_footprint();
    run_test_song_full_hierarchy();

    // Save/Load tests
    run_test_song_save_empty();
    run_test_song_save_load_roundtrip();
    run_test_song_load_nonexistent();
    run_test_song_load_invalid_json();
    run_test_song_load_wrong_version();
    run_test_song_sparse_format();

    // Summary
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total:  " << test_count << std::endl;
    std::cout << "Passed: " << pass_count << std::endl;
    std::cout << "Failed: " << fail_count << std::endl;
    std::cout << "========================================" << std::endl;

    return (fail_count == 0) ? 0 : 1;
}
