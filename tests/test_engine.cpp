/**
 * Unit tests for Engine class
 *
 * Tests the main playback engine:
 * - Start/stop playback
 * - Tempo control
 * - Step progression
 * - Mode/pattern/track switching
 * - LED patterns
 */

#include "../src/core/engine.h"
#include "../src/core/song.h"
#include "../src/lua_bridge/mode_loader.h"
#include <iostream>
#include <cassert>

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

    bool readButton(int button) override {
        (void)button;
        return false;
    }

    uint8_t readRotaryPot(int pot) override {
        // Return values matching Engine defaults to prevent handleInput() from changing state
        // Engine starts at: mode=1, tempo=120 BPM, pattern=0, track=0
        if (pot == 0) return 9;   // R1: Mode 1 (9 * 15 / 128 = 1.05 ≈ 1)
        if (pot == 1) return 42;  // R2: 120 BPM (60 + 42*180/127 = 119.5 ≈ 120)
        if (pot == 2) return 0;   // R3: Pattern 0
        if (pot == 3) return 0;   // R4: Track 0
        return 0;
    }

    uint8_t readSliderPot(int pot) override {
        (void)pot;
        return 0;
    }

    void sendMidiMessage(const MidiMessage& msg) override {
        sent_messages_.push_back(msg);
    }

    void setLED(bool on) override {
        led_state_ = on;
        led_changes_.push_back(on);
    }

    bool getLED() const override {
        return led_state_;
    }

    uint32_t getMillis() override {
        return current_time_;
    }

    void update() override {}

    // Test helpers
    void advanceTime(uint32_t ms) {
        current_time_ += ms;
    }

    void setTime(uint32_t ms) {
        current_time_ = ms;
    }

    const std::vector<MidiMessage>& getSentMessages() const {
        return sent_messages_;
    }

    const std::vector<bool>& getLEDChanges() const {
        return led_changes_;
    }

    void clearMessages() {
        sent_messages_.clear();
    }

    void clearLEDChanges() {
        led_changes_.clear();
    }

private:
    uint32_t current_time_;
    bool led_state_;
    std::vector<MidiMessage> sent_messages_;
    std::vector<bool> led_changes_;
};

// ============================================================================
// Engine Tests
// ============================================================================

TEST(engine_initial_state) {
    Song song;
    MockHardware hw;
    ModeLoader mode_loader;
    Engine engine(&song, &hw, &mode_loader);

    ASSERT_FALSE(engine.isPlaying());
    ASSERT_EQ(engine.getTempo(), 120);  // Default tempo
    ASSERT_EQ(engine.getCurrentMode(), 1);  // Starts at mode 1 (drums)
    ASSERT_EQ(engine.getCurrentPattern(), 0);
    ASSERT_EQ(engine.getCurrentTrack(), 0);
    ASSERT_EQ(engine.getCurrentStep(), 0);
}

TEST(engine_start_stop) {
    Song song;
    MockHardware hw;
    ModeLoader mode_loader;
    Engine engine(&song, &hw, &mode_loader);

    ASSERT_FALSE(engine.isPlaying());

    engine.start();
    ASSERT_TRUE(engine.isPlaying());

    engine.stop();
    ASSERT_FALSE(engine.isPlaying());
}

TEST(engine_tempo_change) {
    Song song;
    MockHardware hw;
    ModeLoader mode_loader;
    Engine engine(&song, &hw, &mode_loader);

    engine.setTempo(140);
    ASSERT_EQ(engine.getTempo(), 140);

    engine.setTempo(90);
    ASSERT_EQ(engine.getTempo(), 90);
}

TEST(engine_tempo_clamping) {
    Song song;
    MockHardware hw;
    ModeLoader mode_loader;
    Engine engine(&song, &hw, &mode_loader);

    // Test lower bound (should clamp to 1)
    engine.setTempo(0);
    ASSERT_EQ(engine.getTempo(), 1);

    engine.setTempo(-50);
    ASSERT_EQ(engine.getTempo(), 1);

    // Test upper bound (should clamp to 1000)
    engine.setTempo(2000);
    ASSERT_EQ(engine.getTempo(), 1000);
}

TEST(engine_mode_switching) {
    Song song;
    MockHardware hw;
    ModeLoader mode_loader;
    Engine engine(&song, &hw, &mode_loader);

    engine.setMode(5);
    ASSERT_EQ(engine.getCurrentMode(), 5);

    engine.setMode(14);
    ASSERT_EQ(engine.getCurrentMode(), 14);

    engine.setMode(0);
    ASSERT_EQ(engine.getCurrentMode(), 0);
}

TEST(engine_pattern_switching) {
    Song song;
    MockHardware hw;
    ModeLoader mode_loader;
    Engine engine(&song, &hw, &mode_loader);

    engine.setPattern(10);
    ASSERT_EQ(engine.getCurrentPattern(), 10);

    engine.setPattern(31);
    ASSERT_EQ(engine.getCurrentPattern(), 31);

    engine.setPattern(0);
    ASSERT_EQ(engine.getCurrentPattern(), 0);
}

TEST(engine_track_switching) {
    Song song;
    MockHardware hw;
    ModeLoader mode_loader;
    Engine engine(&song, &hw, &mode_loader);

    engine.setTrack(3);
    ASSERT_EQ(engine.getCurrentTrack(), 3);

    engine.setTrack(7);
    ASSERT_EQ(engine.getCurrentTrack(), 7);

    engine.setTrack(0);
    ASSERT_EQ(engine.getCurrentTrack(), 0);
}

TEST(engine_step_progression) {
    Song song;
    MockHardware hw;
    ModeLoader mode_loader;
    Engine engine(&song, &hw, &mode_loader);

    engine.start();
    engine.update();  // Initialize timing
    ASSERT_EQ(engine.getCurrentStep(), 0);

    // At 120 BPM, step interval is (60000 / 120) / 4 = 125ms
    hw.advanceTime(126);  // Slightly more to ensure threshold is crossed
    engine.update();
    ASSERT_EQ(engine.getCurrentStep(), 1);

    hw.advanceTime(126);
    engine.update();
    ASSERT_EQ(engine.getCurrentStep(), 2);

    // Advance through remaining steps
    for (int i = 0; i < 14; i++) {
        hw.advanceTime(126);
        engine.update();
    }
    ASSERT_EQ(engine.getCurrentStep(), 0);  // Should wrap to 0 after step 15
}

TEST(engine_led_tempo_beat) {
    Song song;
    MockHardware hw;
    ModeLoader mode_loader;
    Engine engine(&song, &hw, &mode_loader);

    hw.clearLEDChanges();
    engine.start();

    // Advance time and update to trigger first step (step 0 is a beat)
    // At 120 BPM, step interval is 125ms
    hw.advanceTime(126);
    engine.update();

    // Check that LED was turned on (step 0 is a beat)
    const auto& led_changes = hw.getLEDChanges();
    ASSERT_TRUE(led_changes.size() > 0);
    ASSERT_TRUE(led_changes[0]);  // First change should be turning on
}

TEST(engine_led_pattern_saving) {
    Song song;
    MockHardware hw;
    ModeLoader mode_loader;
    Engine engine(&song, &hw, &mode_loader);

    hw.clearLEDChanges();
    engine.triggerLEDPattern(Engine::LEDPattern::SAVING);

    // LED should turn on immediately
    ASSERT_TRUE(hw.getLED());

    // Advance through SAVING pattern (5 blinks, 100ms on/off each)
    for (int i = 0; i < 10; i++) {
        hw.advanceTime(100);
        engine.update();
    }

    // After 1 second, should return to TEMPO_BEAT and turn off
    ASSERT_FALSE(hw.getLED());
}

TEST(engine_led_pattern_error) {
    Song song;
    MockHardware hw;
    ModeLoader mode_loader;
    Engine engine(&song, &hw, &mode_loader);

    hw.clearLEDChanges();
    engine.triggerLEDPattern(Engine::LEDPattern::ERROR);

    // LED should turn on immediately
    ASSERT_TRUE(hw.getLED());

    // Advance through ERROR pattern (3 blinks, 50ms on/off each)
    for (int i = 0; i < 6; i++) {
        hw.advanceTime(50);
        engine.update();
    }

    // After 300ms, should return to TEMPO_BEAT and turn off
    hw.advanceTime(50);
    engine.update();
    ASSERT_FALSE(hw.getLED());
}

TEST(engine_edit_current_event) {
    Song song;
    MockHardware hw;
    ModeLoader mode_loader;
    Engine engine(&song, &hw, &mode_loader);

    // Toggle switch on
    engine.toggleCurrentSwitch();

    // Get the event from song and verify
    const Event& evt = song.getMode(engine.getCurrentMode())
                           .getPattern(engine.getCurrentPattern())
                           .getEvent(engine.getCurrentTrack(), engine.getCurrentStep());
    ASSERT_TRUE(evt.getSwitch());

    // Toggle switch off
    engine.toggleCurrentSwitch();
    ASSERT_FALSE(evt.getSwitch());
}

TEST(engine_set_current_pot) {
    Song song;
    MockHardware hw;
    ModeLoader mode_loader;
    Engine engine(&song, &hw, &mode_loader);

    // Set pot values
    engine.setCurrentPot(0, 100);
    engine.setCurrentPot(1, 50);
    engine.setCurrentPot(2, 75);
    engine.setCurrentPot(3, 25);

    // Get the event from song and verify
    const Event& evt = song.getMode(engine.getCurrentMode())
                           .getPattern(engine.getCurrentPattern())
                           .getEvent(engine.getCurrentTrack(), engine.getCurrentStep());

    ASSERT_EQ(evt.getPot(0), 100);
    ASSERT_EQ(evt.getPot(1), 50);
    ASSERT_EQ(evt.getPot(2), 75);
    ASSERT_EQ(evt.getPot(3), 25);
}

TEST(engine_midi_start_message) {
    Song song;
    MockHardware hw;
    ModeLoader mode_loader;
    Engine engine(&song, &hw, &mode_loader);

    hw.clearMessages();
    engine.start();

    // Should send MIDI start message
    const auto& messages = hw.getSentMessages();
    ASSERT_TRUE(messages.size() > 0);
    ASSERT_EQ(messages[0].data[0], 0xFA);  // MIDI Start
}

TEST(engine_midi_stop_message) {
    Song song;
    MockHardware hw;
    ModeLoader mode_loader;
    Engine engine(&song, &hw, &mode_loader);

    engine.start();
    hw.clearMessages();
    engine.stop();

    // Should send MIDI stop message
    const auto& messages = hw.getSentMessages();
    ASSERT_TRUE(messages.size() > 0);
    ASSERT_EQ(messages[0].data[0], 0xFC);  // MIDI Stop
}

TEST(engine_midi_clock_generation) {
    Song song;
    MockHardware hw;
    ModeLoader mode_loader;
    Engine engine(&song, &hw, &mode_loader);

    engine.start();
    engine.update();  // Initialize timing
    hw.clearMessages();

    // At 120 BPM, clock interval is (60000 / 120) / 24 = 20.83ms
    hw.advanceTime(22);  // Slightly more to ensure threshold is crossed
    engine.update();

    // Should send MIDI clock message
    const auto& messages = hw.getSentMessages();
    ASSERT_TRUE(messages.size() > 0);
    ASSERT_EQ(messages[0].data[0], 0xF8);  // MIDI Clock
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "GRUVBOK Engine Tests" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;

    run_test_engine_initial_state();
    run_test_engine_start_stop();
    run_test_engine_tempo_change();
    run_test_engine_tempo_clamping();
    run_test_engine_mode_switching();
    run_test_engine_pattern_switching();
    run_test_engine_track_switching();
    run_test_engine_step_progression();
    run_test_engine_led_tempo_beat();
    run_test_engine_led_pattern_saving();
    run_test_engine_led_pattern_error();
    run_test_engine_edit_current_event();
    run_test_engine_set_current_pot();
    run_test_engine_midi_start_message();
    run_test_engine_midi_stop_message();
    run_test_engine_midi_clock_generation();

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
