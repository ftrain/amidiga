/**
 * Unit tests for Event class (bit-packing)
 *
 * Tests the 29-bit event structure:
 * - 1 bit: switch (on/off)
 * - 4 × 7 bits: pot values (0-127)
 */

#include "../src/core/event.h"
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
// Event Tests
// ============================================================================

TEST(event_default_constructor) {
    Event evt;
    ASSERT_FALSE(evt.getSwitch());
    ASSERT_EQ(evt.getPot(0), 0);
    ASSERT_EQ(evt.getPot(1), 0);
    ASSERT_EQ(evt.getPot(2), 0);
    ASSERT_EQ(evt.getPot(3), 0);
}

TEST(event_set_get_switch) {
    Event evt;

    evt.setSwitch(true);
    ASSERT_TRUE(evt.getSwitch());

    evt.setSwitch(false);
    ASSERT_FALSE(evt.getSwitch());
}

TEST(event_set_get_pot) {
    Event evt;

    evt.setPot(0, 127);
    ASSERT_EQ(evt.getPot(0), 127);

    evt.setPot(1, 64);
    ASSERT_EQ(evt.getPot(1), 64);

    evt.setPot(2, 0);
    ASSERT_EQ(evt.getPot(2), 0);

    evt.setPot(3, 100);
    ASSERT_EQ(evt.getPot(3), 100);
}

TEST(event_pot_clamping) {
    Event evt;

    // Test values > 127 are clamped
    evt.setPot(0, 255);
    ASSERT_EQ(evt.getPot(0), 127);

    evt.setPot(1, 200);
    ASSERT_EQ(evt.getPot(1), 127);
}

TEST(event_pot_isolation) {
    // Test that setting one pot doesn't affect others
    Event evt;

    evt.setPot(0, 10);
    evt.setPot(1, 20);
    evt.setPot(2, 30);
    evt.setPot(3, 40);

    ASSERT_EQ(evt.getPot(0), 10);
    ASSERT_EQ(evt.getPot(1), 20);
    ASSERT_EQ(evt.getPot(2), 30);
    ASSERT_EQ(evt.getPot(3), 40);

    // Change one pot
    evt.setPot(1, 100);

    // Others should remain unchanged
    ASSERT_EQ(evt.getPot(0), 10);
    ASSERT_EQ(evt.getPot(1), 100);
    ASSERT_EQ(evt.getPot(2), 30);
    ASSERT_EQ(evt.getPot(3), 40);
}

TEST(event_switch_and_pots) {
    // Test that switch and pots are independent
    Event evt;

    evt.setSwitch(true);
    evt.setPot(0, 50);
    evt.setPot(1, 60);
    evt.setPot(2, 70);
    evt.setPot(3, 80);

    ASSERT_TRUE(evt.getSwitch());
    ASSERT_EQ(evt.getPot(0), 50);
    ASSERT_EQ(evt.getPot(1), 60);
    ASSERT_EQ(evt.getPot(2), 70);
    ASSERT_EQ(evt.getPot(3), 80);

    // Toggle switch shouldn't affect pots
    evt.setSwitch(false);

    ASSERT_FALSE(evt.getSwitch());
    ASSERT_EQ(evt.getPot(0), 50);
    ASSERT_EQ(evt.getPot(1), 60);
    ASSERT_EQ(evt.getPot(2), 70);
    ASSERT_EQ(evt.getPot(3), 80);
}

TEST(event_all_values_max) {
    // Test all bits set to maximum
    Event evt;

    evt.setSwitch(true);
    evt.setPot(0, 127);
    evt.setPot(1, 127);
    evt.setPot(2, 127);
    evt.setPot(3, 127);

    ASSERT_TRUE(evt.getSwitch());
    ASSERT_EQ(evt.getPot(0), 127);
    ASSERT_EQ(evt.getPot(1), 127);
    ASSERT_EQ(evt.getPot(2), 127);
    ASSERT_EQ(evt.getPot(3), 127);
}

TEST(event_bit_packing_size) {
    // Ensure Event fits in 32 bits
    ASSERT_EQ(sizeof(Event), sizeof(uint32_t));
}

TEST(event_copy) {
    Event evt1;
    evt1.setSwitch(true);
    evt1.setPot(0, 11);
    evt1.setPot(1, 22);
    evt1.setPot(2, 33);
    evt1.setPot(3, 44);

    Event evt2 = evt1;

    ASSERT_TRUE(evt2.getSwitch());
    ASSERT_EQ(evt2.getPot(0), 11);
    ASSERT_EQ(evt2.getPot(1), 22);
    ASSERT_EQ(evt2.getPot(2), 33);
    ASSERT_EQ(evt2.getPot(3), 44);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "GRUVBOK Event Tests" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;

    // Run all tests
    run_test_event_default_constructor();
    run_test_event_set_get_switch();
    run_test_event_set_get_pot();
    run_test_event_pot_clamping();
    run_test_event_pot_isolation();
    run_test_event_switch_and_pots();
    run_test_event_all_values_max();
    run_test_event_bit_packing_size();
    run_test_event_copy();

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
