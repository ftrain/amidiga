/**
 * Unit tests for PlaybackState class
 *
 * Tests playback state management:
 * - Tempo changes and timing
 * - Position tracking (mode/pattern/track/step)
 * - Step advancement
 * - Bounds checking
 */

#include "../src/core/playback_state.h"
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
// Mock Hardware for Testing
// ============================================================================

class MockHardware : public HardwareInterface {
public:
    MockHardware() : current_time_(0), led_state_(false) {}

    bool init() override { return true; }
    void shutdown() override {}
    bool readButton(int) override { return false; }
    uint8_t readRotaryPot(int) override { return 0; }
    uint8_t readSliderPot(int) override { return 0; }
    void sendMidiMessage(const MidiMessage&) override {}
    void setLED(bool on) override { led_state_ = on; }
    bool getLED() const override { return led_state_; }

    uint32_t getMillis() override { return current_time_; }

    // Test helpers
    void advanceTime(uint32_t ms) { current_time_ += ms; }
    void setTime(uint32_t ms) { current_time_ = ms; }

private:
    uint32_t current_time_;
    bool led_state_;
};

// ============================================================================
// Tests
// ============================================================================

TEST(playback_state_construction) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    // Default state
    ASSERT_FALSE(state.isPlaying());
    ASSERT_EQ(state.getTempo(), 120);
    ASSERT_EQ(state.getCurrentMode(), 1);     // Start with mode 1 (drums)
    ASSERT_EQ(state.getCurrentPattern(), 0);
    ASSERT_EQ(state.getCurrentTrack(), 0);
    ASSERT_EQ(state.getCurrentStep(), 0);
    ASSERT_EQ(state.getTargetMode(), 1);      // Default target mode
}

TEST(start_stop) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    ASSERT_FALSE(state.isPlaying());

    state.start();
    ASSERT_TRUE(state.isPlaying());

    state.stop();
    ASSERT_FALSE(state.isPlaying());
}

TEST(start_resets_step) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    state.start();
    hardware.setTime(1000);
    state.advanceStep(1000);

    ASSERT_EQ(state.getCurrentStep(), 1);

    state.start();  // Start again
    ASSERT_EQ(state.getCurrentStep(), 0);  // Should reset
}

TEST(tempo_setter_and_getter) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    state.setTempo(180);
    ASSERT_EQ(state.getTempo(), 180);

    state.setTempo(60);
    ASSERT_EQ(state.getTempo(), 60);
}

TEST(tempo_clamping_min) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    state.setTempo(0);    // Below minimum
    ASSERT_EQ(state.getTempo(), 1);  // Should clamp to 1

    state.setTempo(-100);  // Negative
    ASSERT_EQ(state.getTempo(), 1);  // Should clamp to 1
}

TEST(tempo_clamping_max) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    state.setTempo(1500);  // Above maximum
    ASSERT_EQ(state.getTempo(), 1000);  // Should clamp to 1000

    state.setTempo(9999);  // Way above maximum
    ASSERT_EQ(state.getTempo(), 1000);  // Should clamp to 1000
}

TEST(step_interval_calculation_120bpm) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    // At 120 BPM:
    // - 1 beat = 500ms
    // - 16 steps per 4 beats
    // - 1 step = 125ms

    state.setTempo(120);
    ASSERT_EQ(state.getStepIntervalMs(), 125);
}

TEST(step_interval_calculation_60bpm) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    // At 60 BPM:
    // - 1 beat = 1000ms
    // - 1 step = 250ms

    state.setTempo(60);
    ASSERT_EQ(state.getStepIntervalMs(), 250);
}

TEST(step_interval_calculation_240bpm) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    // At 240 BPM:
    // - 1 beat = 250ms
    // - 1 step = 62.5ms (rounds to 62)

    state.setTempo(240);
    ASSERT_EQ(state.getStepIntervalMs(), 62);
}

TEST(should_advance_step_timing) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    state.setTempo(120);  // 125ms per step
    state.start();

    // Should not advance immediately
    ASSERT_FALSE(state.shouldAdvanceStep(hardware.getMillis()));

    // Should not advance before interval elapsed
    hardware.advanceTime(100);
    ASSERT_FALSE(state.shouldAdvanceStep(hardware.getMillis()));

    // Should advance after interval elapsed
    hardware.advanceTime(25);  // Total 125ms
    ASSERT_TRUE(state.shouldAdvanceStep(hardware.getMillis()));
}

TEST(should_not_advance_when_stopped) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    state.setTempo(120);
    state.stop();  // Not playing

    hardware.advanceTime(1000);  // Plenty of time

    ASSERT_FALSE(state.shouldAdvanceStep(hardware.getMillis()));
}

TEST(advance_step_increments_position) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    state.start();

    ASSERT_EQ(state.getCurrentStep(), 0);

    hardware.setTime(100);
    state.advanceStep(100);
    ASSERT_EQ(state.getCurrentStep(), 1);

    hardware.setTime(200);
    state.advanceStep(200);
    ASSERT_EQ(state.getCurrentStep(), 2);
}

TEST(advance_step_wraps_at_16) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    state.start();

    // Advance 16 times (should wrap to 0)
    for (int i = 0; i < 16; ++i) {
        hardware.advanceTime(125);
        state.advanceStep(hardware.getMillis());
    }

    ASSERT_EQ(state.getCurrentStep(), 0);  // Wrapped
}

TEST(mode_setter_and_getter) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    state.setMode(5);
    ASSERT_EQ(state.getCurrentMode(), 5);

    state.setMode(0);
    ASSERT_EQ(state.getCurrentMode(), 0);

    state.setMode(14);
    ASSERT_EQ(state.getCurrentMode(), 14);
}

TEST(mode_bounds_checking) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    state.setMode(1);
    ASSERT_EQ(state.getCurrentMode(), 1);

    // Try invalid modes (should be ignored)
    state.setMode(-1);
    ASSERT_EQ(state.getCurrentMode(), 1);  // Unchanged

    state.setMode(15);  // Out of range (valid: 0-14)
    ASSERT_EQ(state.getCurrentMode(), 1);  // Unchanged

    state.setMode(100);
    ASSERT_EQ(state.getCurrentMode(), 1);  // Unchanged
}

TEST(pattern_setter_and_getter) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    state.setPattern(10);
    ASSERT_EQ(state.getCurrentPattern(), 10);

    state.setPattern(0);
    ASSERT_EQ(state.getCurrentPattern(), 0);

    state.setPattern(31);
    ASSERT_EQ(state.getCurrentPattern(), 31);
}

TEST(pattern_bounds_checking) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    state.setPattern(5);
    ASSERT_EQ(state.getCurrentPattern(), 5);

    // Try invalid patterns (should be ignored)
    state.setPattern(-1);
    ASSERT_EQ(state.getCurrentPattern(), 5);  // Unchanged

    state.setPattern(32);  // Out of range (valid: 0-31)
    ASSERT_EQ(state.getCurrentPattern(), 5);  // Unchanged

    state.setPattern(100);
    ASSERT_EQ(state.getCurrentPattern(), 5);  // Unchanged
}

TEST(track_setter_and_getter) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    state.setTrack(3);
    ASSERT_EQ(state.getCurrentTrack(), 3);

    state.setTrack(0);
    ASSERT_EQ(state.getCurrentTrack(), 0);

    state.setTrack(7);
    ASSERT_EQ(state.getCurrentTrack(), 7);
}

TEST(track_bounds_checking) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    state.setTrack(2);
    ASSERT_EQ(state.getCurrentTrack(), 2);

    // Try invalid tracks (should be ignored)
    state.setTrack(-1);
    ASSERT_EQ(state.getCurrentTrack(), 2);  // Unchanged

    state.setTrack(8);  // Out of range (valid: 0-7)
    ASSERT_EQ(state.getCurrentTrack(), 2);  // Unchanged

    state.setTrack(100);
    ASSERT_EQ(state.getCurrentTrack(), 2);  // Unchanged
}

TEST(target_mode_setter_and_getter) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    state.setTargetMode(5);
    ASSERT_EQ(state.getTargetMode(), 5);

    state.setTargetMode(1);
    ASSERT_EQ(state.getTargetMode(), 1);

    state.setTargetMode(14);
    ASSERT_EQ(state.getTargetMode(), 14);
}

TEST(target_mode_bounds_checking) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    state.setTargetMode(3);
    ASSERT_EQ(state.getTargetMode(), 3);

    // Try invalid target modes (should be ignored)
    state.setTargetMode(0);   // Mode 0 not allowed for target
    ASSERT_EQ(state.getTargetMode(), 3);  // Unchanged

    state.setTargetMode(-1);
    ASSERT_EQ(state.getTargetMode(), 3);  // Unchanged

    state.setTargetMode(15);  // Out of range (valid: 1-14)
    ASSERT_EQ(state.getTargetMode(), 3);  // Unchanged

    state.setTargetMode(100);
    ASSERT_EQ(state.getTargetMode(), 3);  // Unchanged
}

TEST(lua_reinit_pending_after_tempo_change) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    hardware.setTime(0);

    // Not pending initially
    ASSERT_FALSE(state.isLuaReinitPending(hardware.getMillis()));

    // Set tempo triggers pending flag
    state.setTempo(180);

    // Still not pending immediately (debouncing)
    ASSERT_FALSE(state.isLuaReinitPending(hardware.getMillis()));

    // Not pending after 500ms (debounce is 1000ms)
    hardware.advanceTime(500);
    ASSERT_FALSE(state.isLuaReinitPending(hardware.getMillis()));

    // Pending after 1000ms (debounce elapsed)
    hardware.advanceTime(500);  // Total 1000ms
    ASSERT_TRUE(state.isLuaReinitPending(hardware.getMillis()));
}

TEST(clear_lua_reinit_pending) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    hardware.setTime(0);
    state.setTempo(180);

    // Wait for debounce
    hardware.advanceTime(1000);
    ASSERT_TRUE(state.isLuaReinitPending(hardware.getMillis()));

    // Clear flag
    state.clearLuaReinitPending();
    ASSERT_FALSE(state.isLuaReinitPending(hardware.getMillis()));
}

TEST(multiple_tempo_changes_debounce) {
    MockHardware hardware;
    PlaybackState state(&hardware);

    hardware.setTime(0);

    // First tempo change
    state.setTempo(180);
    hardware.advanceTime(500);

    // Second tempo change (resets debounce)
    state.setTempo(90);
    hardware.advanceTime(500);

    // Should NOT be pending yet (only 500ms since last change)
    ASSERT_FALSE(state.isLuaReinitPending(hardware.getMillis()));

    // After another 500ms (total 1000ms since last change)
    hardware.advanceTime(500);
    ASSERT_TRUE(state.isLuaReinitPending(hardware.getMillis()));
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "=== PlaybackState Tests ===" << std::endl;

    run_test_playback_state_construction();
    run_test_start_stop();
    run_test_start_resets_step();
    run_test_tempo_setter_and_getter();
    run_test_tempo_clamping_min();
    run_test_tempo_clamping_max();
    run_test_step_interval_calculation_120bpm();
    run_test_step_interval_calculation_60bpm();
    run_test_step_interval_calculation_240bpm();
    run_test_should_advance_step_timing();
    run_test_should_not_advance_when_stopped();
    run_test_advance_step_increments_position();
    run_test_advance_step_wraps_at_16();
    run_test_mode_setter_and_getter();
    run_test_mode_bounds_checking();
    run_test_pattern_setter_and_getter();
    run_test_pattern_bounds_checking();
    run_test_track_setter_and_getter();
    run_test_track_bounds_checking();
    run_test_target_mode_setter_and_getter();
    run_test_target_mode_bounds_checking();
    run_test_lua_reinit_pending_after_tempo_change();
    run_test_clear_lua_reinit_pending();
    run_test_multiple_tempo_changes_debounce();

    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Total: " << test_count << std::endl;
    std::cout << "Pass:  " << pass_count << std::endl;
    std::cout << "Fail:  " << fail_count << std::endl;

    return (fail_count == 0) ? 0 : 1;
}
