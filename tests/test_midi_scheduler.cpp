/**
 * Unit tests for MidiScheduler class
 *
 * Tests MIDI event scheduling with delta timing:
 * - Event queueing and timing
 * - MIDI message creation helpers
 * - Clock and transport messages
 */

#include "../src/hardware/midi_scheduler.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>

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

    bool init() override {
        return true;
    }

    void shutdown() override {
        // No-op for mock
    }

    bool readButton(int button) override {
        (void)button;
        return false;
    }

    uint8_t readRotaryPot(int pot) override {
        (void)pot;
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
    }

    bool getLED() const override {
        return led_state_;
    }

    uint32_t getMillis() override {
        return current_time_;
    }

    void update() override {
        // No-op for mock
    }

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

    void clearMessages() {
        sent_messages_.clear();
    }

private:
    uint32_t current_time_;
    bool led_state_;
    std::vector<MidiMessage> sent_messages_;
};

// ============================================================================
// ScheduledMidiEvent Tests
// ============================================================================

TEST(scheduled_event_note_on) {
    auto evt = MidiScheduler::noteOn(60, 127, 0, 100);

    ASSERT_EQ(evt.data.size(), 3u);
    ASSERT_EQ(evt.data[0], 0x90);  // Note On, channel 0
    ASSERT_EQ(evt.data[1], 60);    // Middle C
    ASSERT_EQ(evt.data[2], 127);   // Max velocity
    ASSERT_EQ(evt.delta_ms, 100u);
    ASSERT_EQ(evt.channel, 0);
}

TEST(scheduled_event_note_off) {
    auto evt = MidiScheduler::noteOff(60, 0, 50);

    ASSERT_EQ(evt.data.size(), 3u);
    ASSERT_EQ(evt.data[0], 0x80);  // Note Off, channel 0
    ASSERT_EQ(evt.data[1], 60);    // Middle C
    ASSERT_EQ(evt.data[2], 0x40);  // Release velocity (64 - common default)
    ASSERT_EQ(evt.delta_ms, 50u);
    ASSERT_EQ(evt.channel, 0);
}

TEST(scheduled_event_control_change) {
    auto evt = MidiScheduler::controlChange(74, 100, 1, 200);

    ASSERT_EQ(evt.data.size(), 3u);
    ASSERT_EQ(evt.data[0], 0xB1);  // CC, channel 1
    ASSERT_EQ(evt.data[1], 74);    // Filter cutoff
    ASSERT_EQ(evt.data[2], 100);   // Value
    ASSERT_EQ(evt.delta_ms, 200u);
    ASSERT_EQ(evt.channel, 1);
}

TEST(scheduled_event_all_notes_off) {
    auto evt = MidiScheduler::allNotesOff(2, 0);

    ASSERT_EQ(evt.data.size(), 3u);
    ASSERT_EQ(evt.data[0], 0xB2);  // CC, channel 2
    ASSERT_EQ(evt.data[1], 123);   // All Notes Off CC
    ASSERT_EQ(evt.data[2], 0);     // Value
    ASSERT_EQ(evt.delta_ms, 0u);
    ASSERT_EQ(evt.channel, 2);
}

TEST(scheduled_event_midi_channels) {
    // Test different MIDI channels
    for (uint8_t ch = 0; ch < 16; ch++) {
        auto evt = MidiScheduler::noteOn(60, 100, ch, 0);
        ASSERT_EQ(evt.channel, ch);
        ASSERT_EQ(evt.data[0], 0x90 | ch);  // Note On + channel
    }
}

// ============================================================================
// MidiScheduler Tests
// ============================================================================

TEST(scheduler_immediate_event) {
    MockHardware hw;
    MidiScheduler scheduler(&hw);

    // Schedule event with no delta (immediate)
    auto evt = MidiScheduler::noteOn(60, 100, 0, 0);
    scheduler.schedule(evt);

    // Update should send it immediately
    scheduler.update();

    const auto& messages = hw.getSentMessages();
    ASSERT_EQ(messages.size(), 1u);
    ASSERT_EQ(messages[0].data[0], 0x90);
    ASSERT_EQ(messages[0].data[1], 60);
}

TEST(scheduler_delayed_event) {
    MockHardware hw;
    MidiScheduler scheduler(&hw);

    // Schedule event 100ms in the future
    auto evt = MidiScheduler::noteOn(60, 100, 0, 100);
    scheduler.schedule(evt);

    // Update immediately - should not send yet
    scheduler.update();
    ASSERT_EQ(hw.getSentMessages().size(), 0u);

    // Advance time by 50ms - still not ready
    hw.advanceTime(50);
    scheduler.update();
    ASSERT_EQ(hw.getSentMessages().size(), 0u);

    // Advance time by another 50ms (total 100ms) - now it should send
    hw.advanceTime(50);
    scheduler.update();
    ASSERT_EQ(hw.getSentMessages().size(), 1u);
}

TEST(scheduler_multiple_events) {
    MockHardware hw;
    MidiScheduler scheduler(&hw);

    // Schedule multiple events with different deltas
    scheduler.schedule(MidiScheduler::noteOn(60, 100, 0, 0));    // Immediate
    scheduler.schedule(MidiScheduler::noteOn(62, 100, 0, 100));  // +100ms
    scheduler.schedule(MidiScheduler::noteOn(64, 100, 0, 200));  // +200ms

    // Update immediately - should send first event
    scheduler.update();
    ASSERT_EQ(hw.getSentMessages().size(), 1u);
    ASSERT_EQ(hw.getSentMessages()[0].data[1], 60);

    hw.clearMessages();

    // Advance 100ms - should send second event
    hw.advanceTime(100);
    scheduler.update();
    ASSERT_EQ(hw.getSentMessages().size(), 1u);
    ASSERT_EQ(hw.getSentMessages()[0].data[1], 62);

    hw.clearMessages();

    // Advance another 100ms (total 200ms) - should send third event
    hw.advanceTime(100);
    scheduler.update();
    ASSERT_EQ(hw.getSentMessages().size(), 1u);
    ASSERT_EQ(hw.getSentMessages()[0].data[1], 64);
}

TEST(scheduler_batch_events) {
    MockHardware hw;
    MidiScheduler scheduler(&hw);

    // Schedule batch of events
    std::vector<ScheduledMidiEvent> events;
    events.push_back(MidiScheduler::noteOn(60, 100, 0, 0));
    events.push_back(MidiScheduler::noteOff(60, 0, 100));
    events.push_back(MidiScheduler::noteOn(62, 100, 0, 200));
    scheduler.schedule(events);

    // First event (immediate)
    scheduler.update();
    ASSERT_EQ(hw.getSentMessages().size(), 1u);

    // Second event (+100ms)
    hw.advanceTime(100);
    hw.clearMessages();
    scheduler.update();
    ASSERT_EQ(hw.getSentMessages().size(), 1u);

    // Third event (+200ms total)
    hw.advanceTime(100);
    hw.clearMessages();
    scheduler.update();
    ASSERT_EQ(hw.getSentMessages().size(), 1u);
}

TEST(scheduler_clear) {
    MockHardware hw;
    MidiScheduler scheduler(&hw);

    // Schedule events
    scheduler.schedule(MidiScheduler::noteOn(60, 100, 0, 100));
    scheduler.schedule(MidiScheduler::noteOn(62, 100, 0, 200));

    // Clear before they fire
    scheduler.clear();

    // Advance time - nothing should be sent
    hw.advanceTime(300);
    scheduler.update();
    ASSERT_EQ(hw.getSentMessages().size(), 0u);
}

TEST(scheduler_clock_message) {
    MockHardware hw;
    MidiScheduler scheduler(&hw);

    scheduler.sendClock();

    const auto& messages = hw.getSentMessages();
    ASSERT_EQ(messages.size(), 1u);
    ASSERT_EQ(messages[0].data[0], 0xF8);  // MIDI Clock
}

TEST(scheduler_start_message) {
    MockHardware hw;
    MidiScheduler scheduler(&hw);

    scheduler.sendStart();

    const auto& messages = hw.getSentMessages();
    ASSERT_EQ(messages.size(), 1u);
    ASSERT_EQ(messages[0].data[0], 0xFA);  // MIDI Start
}

TEST(scheduler_stop_message) {
    MockHardware hw;
    MidiScheduler scheduler(&hw);

    scheduler.sendStop();

    const auto& messages = hw.getSentMessages();
    ASSERT_EQ(messages.size(), 1u);
    ASSERT_EQ(messages[0].data[0], 0xFC);  // MIDI Stop
}

TEST(scheduler_continue_message) {
    MockHardware hw;
    MidiScheduler scheduler(&hw);

    scheduler.sendContinue();

    const auto& messages = hw.getSentMessages();
    ASSERT_EQ(messages.size(), 1u);
    ASSERT_EQ(messages[0].data[0], 0xFB);  // MIDI Continue
}

TEST(scheduler_event_ordering) {
    // Test that events are sent in correct time order, even if scheduled out of order
    MockHardware hw;
    MidiScheduler scheduler(&hw);

    // Schedule in reverse time order
    scheduler.schedule(MidiScheduler::noteOn(64, 100, 0, 200));  // Last
    scheduler.schedule(MidiScheduler::noteOn(62, 100, 0, 100));  // Middle
    scheduler.schedule(MidiScheduler::noteOn(60, 100, 0, 0));    // First

    // Should send in time order (60, then 62, then 64)
    scheduler.update();
    ASSERT_EQ(hw.getSentMessages().size(), 1u);
    ASSERT_EQ(hw.getSentMessages()[0].data[1], 60);

    hw.clearMessages();
    hw.advanceTime(100);
    scheduler.update();
    ASSERT_EQ(hw.getSentMessages().size(), 1u);
    ASSERT_EQ(hw.getSentMessages()[0].data[1], 62);

    hw.clearMessages();
    hw.advanceTime(100);
    scheduler.update();
    ASSERT_EQ(hw.getSentMessages().size(), 1u);
    ASSERT_EQ(hw.getSentMessages()[0].data[1], 64);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "GRUVBOK MidiScheduler Tests" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;

    // ScheduledMidiEvent tests
    run_test_scheduled_event_note_on();
    run_test_scheduled_event_note_off();
    run_test_scheduled_event_control_change();
    run_test_scheduled_event_all_notes_off();
    run_test_scheduled_event_midi_channels();

    // MidiScheduler tests
    run_test_scheduler_immediate_event();
    run_test_scheduler_delayed_event();
    run_test_scheduler_multiple_events();
    run_test_scheduler_batch_events();
    run_test_scheduler_clear();
    run_test_scheduler_clock_message();
    run_test_scheduler_start_message();
    run_test_scheduler_stop_message();
    run_test_scheduler_continue_message();
    run_test_scheduler_event_ordering();

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
