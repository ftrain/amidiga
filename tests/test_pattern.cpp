/**
 * Unit tests for Pattern and Track classes
 *
 * Tests the data structure hierarchy:
 * - Track: Contains 16 Events (one per button)
 * - Pattern: Contains 8 Tracks
 */

#include "../src/core/pattern.h"
#include <iostream>
#include <cassert>

// Simple test framework (same as test_event.cpp)
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
// Track Tests
// ============================================================================

TEST(track_default_constructor) {
    Track track;

    // All events should be empty (switch off, pots 0)
    for (int i = 0; i < Track::NUM_EVENTS; i++) {
        const Event& evt = track.getEvent(i);
        ASSERT_FALSE(evt.getSwitch());
        ASSERT_EQ(evt.getPot(0), 0);
        ASSERT_EQ(evt.getPot(1), 0);
        ASSERT_EQ(evt.getPot(2), 0);
        ASSERT_EQ(evt.getPot(3), 0);
    }
}

TEST(track_set_get_event) {
    Track track;

    Event evt;
    evt.setSwitch(true);
    evt.setPot(0, 10);
    evt.setPot(1, 20);
    evt.setPot(2, 30);
    evt.setPot(3, 40);

    track.setEvent(5, evt);

    const Event& retrieved = track.getEvent(5);
    ASSERT_TRUE(retrieved.getSwitch());
    ASSERT_EQ(retrieved.getPot(0), 10);
    ASSERT_EQ(retrieved.getPot(1), 20);
    ASSERT_EQ(retrieved.getPot(2), 30);
    ASSERT_EQ(retrieved.getPot(3), 40);

    // Other events should remain empty
    const Event& evt0 = track.getEvent(0);
    ASSERT_FALSE(evt0.getSwitch());
}

TEST(track_event_isolation) {
    // Test that events in a track don't affect each other
    Track track;

    for (int i = 0; i < Track::NUM_EVENTS; i++) {
        Event evt;
        evt.setSwitch(i % 2 == 0);  // Alternate on/off
        evt.setPot(0, i * 8);
        track.setEvent(i, evt);
    }

    // Verify each event
    for (int i = 0; i < Track::NUM_EVENTS; i++) {
        const Event& evt = track.getEvent(i);
        ASSERT_EQ(evt.getSwitch(), i % 2 == 0);
        ASSERT_EQ(evt.getPot(0), i * 8);
    }
}

TEST(track_clear) {
    Track track;

    // Set some events
    for (int i = 0; i < Track::NUM_EVENTS; i++) {
        Event evt;
        evt.setSwitch(true);
        evt.setPot(0, 127);
        track.setEvent(i, evt);
    }

    // Clear track
    track.clear();

    // All events should be empty
    for (int i = 0; i < Track::NUM_EVENTS; i++) {
        const Event& evt = track.getEvent(i);
        ASSERT_FALSE(evt.getSwitch());
        ASSERT_EQ(evt.getPot(0), 0);
    }
}

TEST(track_num_events_constant) {
    // Verify the constant matches the hardware (16 buttons)
    ASSERT_EQ(Track::NUM_EVENTS, 16);
}

// ============================================================================
// Pattern Tests
// ============================================================================

TEST(pattern_default_constructor) {
    Pattern pattern;

    // All tracks should be empty
    for (int track_num = 0; track_num < Pattern::NUM_TRACKS; track_num++) {
        const Track& track = pattern.getTrack(track_num);
        for (int step = 0; step < Track::NUM_EVENTS; step++) {
            const Event& evt = track.getEvent(step);
            ASSERT_FALSE(evt.getSwitch());
        }
    }
}

TEST(pattern_get_track) {
    Pattern pattern;

    // Get a track and modify it
    Track& track = pattern.getTrack(3);

    Event evt;
    evt.setSwitch(true);
    evt.setPot(0, 99);
    track.setEvent(7, evt);

    // Retrieve via pattern and verify
    const Event& retrieved = pattern.getEvent(3, 7);
    ASSERT_TRUE(retrieved.getSwitch());
    ASSERT_EQ(retrieved.getPot(0), 99);
}

TEST(pattern_set_get_event) {
    Pattern pattern;

    Event evt;
    evt.setSwitch(true);
    evt.setPot(0, 50);
    evt.setPot(1, 60);
    evt.setPot(2, 70);
    evt.setPot(3, 80);

    pattern.setEvent(2, 10, evt);

    const Event& retrieved = pattern.getEvent(2, 10);
    ASSERT_TRUE(retrieved.getSwitch());
    ASSERT_EQ(retrieved.getPot(0), 50);
    ASSERT_EQ(retrieved.getPot(1), 60);
    ASSERT_EQ(retrieved.getPot(2), 70);
    ASSERT_EQ(retrieved.getPot(3), 80);
}

TEST(pattern_track_isolation) {
    // Test that tracks don't affect each other
    Pattern pattern;

    for (int track_num = 0; track_num < Pattern::NUM_TRACKS; track_num++) {
        Event evt;
        evt.setSwitch(true);
        evt.setPot(0, track_num * 10);
        pattern.setEvent(track_num, 0, evt);
    }

    // Verify each track
    for (int track_num = 0; track_num < Pattern::NUM_TRACKS; track_num++) {
        const Event& evt = pattern.getEvent(track_num, 0);
        ASSERT_TRUE(evt.getSwitch());
        ASSERT_EQ(evt.getPot(0), track_num * 10);
    }
}

TEST(pattern_clear) {
    Pattern pattern;

    // Set all events
    for (int track_num = 0; track_num < Pattern::NUM_TRACKS; track_num++) {
        for (int step = 0; step < Track::NUM_EVENTS; step++) {
            Event evt;
            evt.setSwitch(true);
            evt.setPot(0, 127);
            pattern.setEvent(track_num, step, evt);
        }
    }

    // Clear pattern
    pattern.clear();

    // All events should be empty
    for (int track_num = 0; track_num < Pattern::NUM_TRACKS; track_num++) {
        for (int step = 0; step < Track::NUM_EVENTS; step++) {
            const Event& evt = pattern.getEvent(track_num, step);
            ASSERT_FALSE(evt.getSwitch());
            ASSERT_EQ(evt.getPot(0), 0);
        }
    }
}

TEST(pattern_num_tracks_constant) {
    // Verify the constant matches the design (8 tracks)
    ASSERT_EQ(Pattern::NUM_TRACKS, 8);
}

TEST(pattern_full_data) {
    // Test filling an entire pattern with unique data
    Pattern pattern;

    for (int track_num = 0; track_num < Pattern::NUM_TRACKS; track_num++) {
        for (int step = 0; step < Track::NUM_EVENTS; step++) {
            Event evt;
            evt.setSwitch((track_num + step) % 2 == 0);
            evt.setPot(0, (track_num * 16 + step) % 128);
            evt.setPot(1, track_num * 10);
            evt.setPot(2, step * 5);
            evt.setPot(3, 100);
            pattern.setEvent(track_num, step, evt);
        }
    }

    // Verify all data
    for (int track_num = 0; track_num < Pattern::NUM_TRACKS; track_num++) {
        for (int step = 0; step < Track::NUM_EVENTS; step++) {
            const Event& evt = pattern.getEvent(track_num, step);
            ASSERT_EQ(evt.getSwitch(), (track_num + step) % 2 == 0);
            ASSERT_EQ(evt.getPot(0), (track_num * 16 + step) % 128);
            ASSERT_EQ(evt.getPot(1), track_num * 10);
            ASSERT_EQ(evt.getPot(2), step * 5);
            ASSERT_EQ(evt.getPot(3), 100);
        }
    }
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "GRUVBOK Pattern/Track Tests" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;

    // Track tests
    run_test_track_default_constructor();
    run_test_track_set_get_event();
    run_test_track_event_isolation();
    run_test_track_clear();
    run_test_track_num_events_constant();

    // Pattern tests
    run_test_pattern_default_constructor();
    run_test_pattern_get_track();
    run_test_pattern_set_get_event();
    run_test_pattern_track_isolation();
    run_test_pattern_clear();
    run_test_pattern_num_tracks_constant();
    run_test_pattern_full_data();

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
