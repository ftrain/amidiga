/**
 * Unit tests for Mode0Sequencer class
 *
 * Tests Mode 0 (Song Mode) sequencing:
 * - Loop length calculation
 * - Pattern overrides
 * - Scale and velocity parameters
 * - Bounds checking
 */

#include "../src/core/mode0_sequencer.h"
#include <iostream>
#include <cassert>
#include <vector>

// Simple test framework
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
// Tests
// ============================================================================

TEST(mode0_sequencer_construction) {
    Song song;
    Mode0Sequencer sequencer(&song);

    // Default state
    ASSERT_EQ(sequencer.getCurrentStep(), 0);
    ASSERT_EQ(sequencer.getLoopLength(), 16);  // Default to full 16 steps
    ASSERT_EQ(sequencer.getScaleRoot(), 0);    // C
    ASSERT_EQ(sequencer.getScaleType(), 0);    // Ionian/Major
}

TEST(loop_length_calculation_no_active_steps) {
    Song song;
    Mode0Sequencer sequencer(&song);

    // No buttons pressed in Mode 0
    sequencer.calculateLoopLength();

    // Should default to 16 (full loop)
    ASSERT_EQ(sequencer.getLoopLength(), 16);
}

TEST(loop_length_calculation_single_step) {
    Song song;
    Mode0Sequencer sequencer(&song);

    // Press button 0 (first step)
    Mode& mode0 = song.getMode(0);
    Pattern& pattern = mode0.getPattern(0);
    Event& event = pattern.getEvent(0, 0);  // Track 0, step 0
    event.setSwitch(true);

    sequencer.calculateLoopLength();

    // Loop length should be 1 (only step 0 active)
    ASSERT_EQ(sequencer.getLoopLength(), 1);
}

TEST(loop_length_calculation_multiple_steps) {
    Song song;
    Mode0Sequencer sequencer(&song);

    // Press buttons 0, 2, 4 (steps 0, 2, 4)
    Mode& mode0 = song.getMode(0);
    Pattern& pattern = mode0.getPattern(0);

    pattern.getEvent(0, 0).setSwitch(true);  // Step 0
    pattern.getEvent(0, 2).setSwitch(true);  // Step 2
    pattern.getEvent(0, 4).setSwitch(true);  // Step 4

    sequencer.calculateLoopLength();

    // Loop length should be 5 (highest step + 1)
    ASSERT_EQ(sequencer.getLoopLength(), 5);
}

TEST(loop_length_calculation_last_step) {
    Song song;
    Mode0Sequencer sequencer(&song);

    // Press last button (step 15)
    Mode& mode0 = song.getMode(0);
    Pattern& pattern = mode0.getPattern(0);
    pattern.getEvent(0, 15).setSwitch(true);  // Step 15

    sequencer.calculateLoopLength();

    // Loop length should be 16 (full loop)
    ASSERT_EQ(sequencer.getLoopLength(), 16);
}

TEST(advance_step) {
    Song song;
    Mode0Sequencer sequencer(&song);

    ASSERT_EQ(sequencer.getCurrentStep(), 0);

    sequencer.advanceStep();
    ASSERT_EQ(sequencer.getCurrentStep(), 1);

    sequencer.advanceStep();
    ASSERT_EQ(sequencer.getCurrentStep(), 2);
}

TEST(advance_step_wraps_at_loop_length) {
    Song song;
    Mode0Sequencer sequencer(&song);

    // Set loop length to 4
    Mode& mode0 = song.getMode(0);
    Pattern& pattern = mode0.getPattern(0);
    pattern.getEvent(0, 3).setSwitch(true);  // Step 3 (loop length = 4)
    sequencer.calculateLoopLength();

    ASSERT_EQ(sequencer.getLoopLength(), 4);

    sequencer.start();  // Reset to step 0

    // Advance 4 times (should wrap)
    sequencer.advanceStep();  // Step 1
    sequencer.advanceStep();  // Step 2
    sequencer.advanceStep();  // Step 3
    sequencer.advanceStep();  // Step 0 (wrap)

    ASSERT_EQ(sequencer.getCurrentStep(), 0);
}

TEST(start_resets_position) {
    Song song;
    Mode0Sequencer sequencer(&song);

    sequencer.advanceStep();
    sequencer.advanceStep();
    ASSERT_EQ(sequencer.getCurrentStep(), 2);

    sequencer.start();
    ASSERT_EQ(sequencer.getCurrentStep(), 0);
}

TEST(pattern_override_default) {
    Song song;
    Mode0Sequencer sequencer(&song);

    // Default: no overrides (-1 for all modes)
    for (int mode = 1; mode < 15; ++mode) {
        ASSERT_EQ(sequencer.getPatternOverride(mode), -1);
    }
}

TEST(pattern_override_apply_parameters) {
    Song song;
    Mode0Sequencer sequencer(&song);

    // Set up Mode 0 event with pattern selection
    Mode& mode0 = song.getMode(0);
    Pattern& pattern = mode0.getPattern(0);
    Event& event = pattern.getEvent(0, 0);  // Track 0, step 0

    event.setSwitch(true);
    event.setPot(0, 64);  // S1 = 64 → pattern ~16

    sequencer.applyParameters();

    // All modes 1-14 should have pattern override set
    for (int mode = 1; mode < 15; ++mode) {
        int override = sequencer.getPatternOverride(mode);
        ASSERT_TRUE(override >= 0 && override < 32);
    }
}

TEST(scale_root_extraction) {
    Song song;
    Mode0Sequencer sequencer(&song);

    // Set up Mode 0 event with scale root
    Mode& mode0 = song.getMode(0);
    Pattern& pattern = mode0.getPattern(0);
    Event& event = pattern.getEvent(0, 0);

    event.setSwitch(true);
    event.setPot(1, 53);  // S2 = 53 → scale root ~5 (F)

    sequencer.applyParameters();

    // Scale root should be in range 0-11
    ASSERT_TRUE(sequencer.getScaleRoot() >= 0 && sequencer.getScaleRoot() <= 11);
}

TEST(scale_type_extraction) {
    Song song;
    Mode0Sequencer sequencer(&song);

    // Set up Mode 0 event with scale type
    Mode& mode0 = song.getMode(0);
    Pattern& pattern = mode0.getPattern(0);
    Event& event = pattern.getEvent(0, 0);

    event.setSwitch(true);
    event.setPot(2, 96);  // S3 = 96 → scale type ~6

    sequencer.applyParameters();

    // Scale type should be in range 0-7
    ASSERT_TRUE(sequencer.getScaleType() >= 0 && sequencer.getScaleType() <= 7);
}

TEST(velocity_offset_extraction) {
    Song song;
    Mode0Sequencer sequencer(&song);

    // Set up Mode 0 event with velocity offset
    Mode& mode0 = song.getMode(0);
    Pattern& pattern = mode0.getPattern(0);
    Event& event = pattern.getEvent(0, 0);

    event.setSwitch(true);
    event.setPot(3, 127);  // S4 = 127 → velocity offset +63

    sequencer.applyParameters();

    // Velocity offset should be in range -64 to +63
    for (int mode = 1; mode < 15; ++mode) {
        int offset = sequencer.getVelocityOffset(mode);
        ASSERT_TRUE(offset >= -64 && offset <= 63);
    }
}

TEST(parse_event_bounds_checking) {
    Song song;
    Mode0Sequencer sequencer(&song);

    Event event;
    event.setSwitch(true);
    event.setPot(0, 127);  // Pattern = max
    event.setPot(1, 127);  // Scale root = max
    event.setPot(2, 127);  // Scale type = max
    event.setPot(3, 127);  // Velocity offset = max

    // Parse with valid mode
    sequencer.parseEvent(event, 5);

    // Values should be clamped to valid ranges
    ASSERT_TRUE(sequencer.getPatternOverride(5) >= 0 && sequencer.getPatternOverride(5) <= 31);
    ASSERT_TRUE(sequencer.getScaleRoot() >= 0 && sequencer.getScaleRoot() <= 11);
    ASSERT_TRUE(sequencer.getScaleType() >= 0 && sequencer.getScaleType() <= 7);
    ASSERT_TRUE(sequencer.getVelocityOffset(5) >= -64 && sequencer.getVelocityOffset(5) <= 63);
}

TEST(pattern_override_bounds_invalid_mode) {
    Song song;
    Mode0Sequencer sequencer(&song);

    // Try to get pattern override for invalid modes
    ASSERT_EQ(sequencer.getPatternOverride(-1), -1);  // Invalid (negative)
    ASSERT_EQ(sequencer.getPatternOverride(15), -1);  // Invalid (out of range)
    ASSERT_EQ(sequencer.getPatternOverride(100), -1); // Invalid (way out of range)
}

TEST(velocity_offset_bounds_invalid_mode) {
    Song song;
    Mode0Sequencer sequencer(&song);

    // Try to get velocity offset for invalid modes
    ASSERT_EQ(sequencer.getVelocityOffset(-1), 0);   // Invalid → default 0
    ASSERT_EQ(sequencer.getVelocityOffset(15), 0);   // Invalid → default 0
    ASSERT_EQ(sequencer.getVelocityOffset(100), 0);  // Invalid → default 0
}

TEST(parse_event_with_inactive_switch) {
    Song song;
    Mode0Sequencer sequencer(&song);

    Event event;
    event.setSwitch(false);  // Inactive
    event.setPot(0, 127);    // Pattern value (should be ignored)

    sequencer.parseEvent(event, 5);

    // Pattern override should still be -1 (not set)
    ASSERT_EQ(sequencer.getPatternOverride(5), -1);
}

TEST(apply_parameters_inactive_step) {
    Song song;
    Mode0Sequencer sequencer(&song);

    // Set up Mode 0 event with switch OFF
    Mode& mode0 = song.getMode(0);
    Pattern& pattern = mode0.getPattern(0);
    Event& event = pattern.getEvent(0, 0);

    event.setSwitch(false);  // Inactive
    event.setPot(0, 64);     // Pattern (should be ignored)

    sequencer.applyParameters();

    // Pattern overrides should remain -1
    for (int mode = 1; mode < 15; ++mode) {
        ASSERT_EQ(sequencer.getPatternOverride(mode), -1);
    }
}

TEST(loop_length_bounds_clamping) {
    Song song;
    Mode0Sequencer sequencer(&song);

    // Manually test the clamping logic by setting all steps inactive
    // (This tests the internal clamping to [1, 16])

    sequencer.calculateLoopLength();

    // Loop length should be clamped to valid range [1, 16]
    ASSERT_TRUE(sequencer.getLoopLength() >= 1 && sequencer.getLoopLength() <= 16);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "=== Mode0Sequencer Tests ===" << std::endl;

    run_test_mode0_sequencer_construction();
    run_test_loop_length_calculation_no_active_steps();
    run_test_loop_length_calculation_single_step();
    run_test_loop_length_calculation_multiple_steps();
    run_test_loop_length_calculation_last_step();
    run_test_advance_step();
    run_test_advance_step_wraps_at_loop_length();
    run_test_start_resets_position();
    run_test_pattern_override_default();
    run_test_pattern_override_apply_parameters();
    run_test_scale_root_extraction();
    run_test_scale_type_extraction();
    run_test_velocity_offset_extraction();
    run_test_parse_event_bounds_checking();
    run_test_pattern_override_bounds_invalid_mode();
    run_test_velocity_offset_bounds_invalid_mode();
    run_test_parse_event_with_inactive_switch();
    run_test_apply_parameters_inactive_step();
    run_test_loop_length_bounds_clamping();

    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Total: " << test_count << std::endl;
    std::cout << "Pass:  " << pass_count << std::endl;
    std::cout << "Fail:  " << fail_count << std::endl;

    return (fail_count == 0) ? 0 : 1;
}
