/**
 * Unit tests for MidiClockManager class
 *
 * Tests MIDI clock timing at 24 PPQN:
 * - Clock interval calculation
 * - Absolute timestamp-based timing
 * - Start/Stop message handling
 * - Tempo changes
 */

#include "../src/core/midi_clock_manager.h"
#include "../src/hardware/midi_scheduler.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <cmath>

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

#define ASSERT_NEAR(a, b, tolerance) \
    if (std::abs((a) - (b)) > (tolerance)) { \
        throw std::runtime_error(std::string("Expected ") + #a + " ≈ " + #b + \
                                 " (tolerance " + std::to_string(tolerance) + ")" + \
                                 ", got " + std::to_string(a) + " vs " + std::to_string(b)); \
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

    void sendMidiMessage(const MidiMessage& msg) override {
        sent_messages_.push_back(msg);
    }

    void setLED(bool on) override { led_state_ = on; }
    bool getLED() const override { return led_state_; }

    uint32_t getMillis() override { return current_time_; }

    // Test helpers
    void advanceTime(uint32_t ms) { current_time_ += ms; }
    void setTime(uint32_t ms) { current_time_ = ms; }
    const std::vector<MidiMessage>& getSentMessages() const { return sent_messages_; }
    void clearMessages() { sent_messages_.clear(); }

    int countClockMessages() const {
        int count = 0;
        for (const auto& msg : sent_messages_) {
            if (msg.data.size() == 1 && msg.data[0] == 0xF8) {
                count++;
            }
        }
        return count;
    }

    bool hasStartMessage() const {
        for (const auto& msg : sent_messages_) {
            if (msg.data.size() == 1 && msg.data[0] == 0xFA) {
                return true;
            }
        }
        return false;
    }

    bool hasStopMessage() const {
        for (const auto& msg : sent_messages_) {
            if (msg.data.size() == 1 && msg.data[0] == 0xFC) {
                return true;
            }
        }
        return false;
    }

private:
    uint32_t current_time_;
    bool led_state_;
    std::vector<MidiMessage> sent_messages_;
};

// ============================================================================
// Tests
// ============================================================================

TEST(clock_manager_construction) {
    MockHardware hardware;
    MidiScheduler scheduler(&hardware);
    MidiClockManager clock_manager(&scheduler, &hardware);

    // Default tempo should be 120 BPM
    ASSERT_EQ(clock_manager.getTempo(), 120);
}

TEST(clock_interval_calculation_120bpm) {
    MockHardware hardware;
    MidiScheduler scheduler(&hardware);
    MidiClockManager clock_manager(&scheduler, &hardware);

    // At 120 BPM:
    // - 1 quarter note = 500ms
    // - 24 PPQN = 500ms / 24 = 20.833ms per clock

    clock_manager.start();
    hardware.clearMessages();

    // Advance by one clock interval (should send 1 clock)
    hardware.advanceTime(21);  // ~20.833ms
    clock_manager.update();

    ASSERT_EQ(hardware.countClockMessages(), 1);
}

TEST(clock_interval_calculation_60bpm) {
    MockHardware hardware;
    MidiScheduler scheduler(&hardware);
    MidiClockManager clock_manager(&scheduler, &hardware);

    clock_manager.setTempo(60);  // Slow tempo

    // At 60 BPM:
    // - 1 quarter note = 1000ms
    // - 24 PPQN = 1000ms / 24 = 41.666ms per clock

    clock_manager.start();
    hardware.clearMessages();

    // Advance by one clock interval
    hardware.advanceTime(42);  // ~41.666ms
    clock_manager.update();

    ASSERT_EQ(hardware.countClockMessages(), 1);
}

TEST(clock_interval_calculation_240bpm) {
    MockHardware hardware;
    MidiScheduler scheduler(&hardware);
    MidiClockManager clock_manager(&scheduler, &hardware);

    clock_manager.setTempo(240);  // Fast tempo

    // At 240 BPM:
    // - 1 quarter note = 250ms
    // - 24 PPQN = 250ms / 24 = 10.416ms per clock

    clock_manager.start();
    hardware.clearMessages();

    // Advance by one clock interval
    hardware.advanceTime(11);  // ~10.416ms
    clock_manager.update();

    ASSERT_EQ(hardware.countClockMessages(), 1);
}

TEST(start_sends_start_message) {
    MockHardware hardware;
    MidiScheduler scheduler(&hardware);
    MidiClockManager clock_manager(&scheduler, &hardware);

    hardware.clearMessages();
    clock_manager.start();
    scheduler.update();

    ASSERT_TRUE(hardware.hasStartMessage());
}

TEST(stop_sends_stop_message) {
    MockHardware hardware;
    MidiScheduler scheduler(&hardware);
    MidiClockManager clock_manager(&scheduler, &hardware);

    clock_manager.start();
    hardware.clearMessages();

    clock_manager.stop();
    scheduler.update();

    ASSERT_TRUE(hardware.hasStopMessage());
}

TEST(multiple_clock_pulses) {
    MockHardware hardware;
    MidiScheduler scheduler(&hardware);
    MidiClockManager clock_manager(&scheduler, &hardware);

    clock_manager.start();
    hardware.clearMessages();

    // At 120 BPM, one quarter note = 500ms = 24 clocks
    // So 500ms should produce exactly 24 clock pulses

    for (int i = 0; i < 24; ++i) {
        hardware.advanceTime(21);  // ~20.833ms per clock
        clock_manager.update();
    }

    // Should have sent 24 clock messages (one quarter note)
    ASSERT_EQ(hardware.countClockMessages(), 24);
}

TEST(clock_catchup_after_delay) {
    MockHardware hardware;
    MidiScheduler scheduler(&hardware);
    MidiClockManager clock_manager(&scheduler, &hardware);

    clock_manager.start();
    hardware.clearMessages();

    // Simulate a delay: advance time by 5 clock intervals at once
    // At 120 BPM, ~20.833ms per clock × 5 = ~104ms
    hardware.advanceTime(104);
    clock_manager.update();

    // Should catch up and send 5 clock messages
    ASSERT_EQ(hardware.countClockMessages(), 5);
}

TEST(tempo_change_updates_interval) {
    MockHardware hardware;
    MidiScheduler scheduler(&hardware);
    MidiClockManager clock_manager(&scheduler, &hardware);

    // Start at 120 BPM
    clock_manager.start();
    hardware.clearMessages();

    // Advance one clock at 120 BPM (~20.833ms)
    hardware.advanceTime(21);
    clock_manager.update();
    ASSERT_EQ(hardware.countClockMessages(), 1);

    hardware.clearMessages();

    // Change to 60 BPM (slower, ~41.666ms per clock)
    clock_manager.stop();
    clock_manager.setTempo(60);
    clock_manager.start();

    hardware.clearMessages();
    hardware.setTime(0);

    // At new tempo, need ~42ms for one clock
    hardware.advanceTime(21);  // Should NOT trigger yet
    clock_manager.update();
    ASSERT_EQ(hardware.countClockMessages(), 0);

    hardware.advanceTime(21);  // Now should trigger (total 42ms)
    clock_manager.update();
    ASSERT_EQ(hardware.countClockMessages(), 1);
}

TEST(absolute_timing_prevents_drift) {
    MockHardware hardware;
    MidiScheduler scheduler(&hardware);
    MidiClockManager clock_manager(&scheduler, &hardware);

    clock_manager.start();
    hardware.clearMessages();

    // Simulate slight timing jitter (±1ms)
    // Over 24 clocks, this shouldn't cause drift

    int total_clocks = 0;
    for (int i = 0; i < 24; ++i) {
        // Add jitter: alternate between 20ms and 22ms
        hardware.advanceTime((i % 2 == 0) ? 20 : 22);
        clock_manager.update();
        total_clocks = hardware.countClockMessages();
    }

    // Despite jitter, should still have exactly 24 clocks
    // (absolute timing compensates for jitter)
    ASSERT_TRUE(total_clocks >= 23 && total_clocks <= 25);
}

TEST(no_clocks_before_start) {
    MockHardware hardware;
    MidiScheduler scheduler(&hardware);
    MidiClockManager clock_manager(&scheduler, &hardware);

    // Don't call start()
    hardware.clearMessages();

    hardware.advanceTime(100);
    clock_manager.update();

    // Should not send any clocks before start()
    ASSERT_EQ(hardware.countClockMessages(), 0);
}

TEST(tempo_getter) {
    MockHardware hardware;
    MidiScheduler scheduler(&hardware);
    MidiClockManager clock_manager(&scheduler, &hardware);

    ASSERT_EQ(clock_manager.getTempo(), 120);

    clock_manager.setTempo(180);
    ASSERT_EQ(clock_manager.getTempo(), 180);

    clock_manager.setTempo(90);
    ASSERT_EQ(clock_manager.getTempo(), 90);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "=== MidiClockManager Tests ===" << std::endl;

    run_test_clock_manager_construction();
    run_test_clock_interval_calculation_120bpm();
    run_test_clock_interval_calculation_60bpm();
    run_test_clock_interval_calculation_240bpm();
    run_test_start_sends_start_message();
    run_test_stop_sends_stop_message();
    run_test_multiple_clock_pulses();
    run_test_clock_catchup_after_delay();
    run_test_tempo_change_updates_interval();
    run_test_absolute_timing_prevents_drift();
    run_test_no_clocks_before_start();
    run_test_tempo_getter();

    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Total: " << test_count << std::endl;
    std::cout << "Pass:  " << pass_count << std::endl;
    std::cout << "Fail:  " << fail_count << std::endl;

    return (fail_count == 0) ? 0 : 1;
}
