#pragma once

#include "../hardware/hardware_base.h"
#include <Arduino.h>
#include <array>

namespace gruvbok {

/**
 * @brief Teensy 4.1 hardware implementation
 *
 * Inherits common functionality from HardwareBase and adds:
 * - GPIO button input with debouncing
 * - ADC pot input with filtering
 * - PWM LED output with brightness control
 * - USB MIDI output
 *
 * Pin Mappings (example - adjust based on actual hardware):
 *
 * Buttons (B1-B16): Digital input pins with pullup (active-low)
 * - B1-B8:   Pins 0-7
 * - B9-B16:  Pins 8, 9, 10, 11, 12, 24, 25, 26
 *
 * Rotary Pots (R1-R4): Analog input pins
 * - R1 (Mode):    A0 (pin 14)
 * - R2 (Tempo):   A1 (pin 15)
 * - R3 (Pattern): A2 (pin 16)
 * - R4 (Track):   A3 (pin 17)
 *
 * Slider Pots (S1-S4): Analog input pins
 * - S1: A4 (pin 18)
 * - S2: A5 (pin 19)
 * - S3: A6 (pin 20)
 * - S4: A7 (pin 21)
 *
 * LED: Digital output pin (PWM capable)
 * - LED: Pin 13 (onboard LED)
 *
 * MIDI: USB MIDI (no additional pins needed)
 *
 * @note Inherits button/pot/LED state management from HardwareBase
 * @note update() actively reads hardware and updates inherited state arrays
 * @see HardwareBase for inherited functionality
 */
class TeensyHardware : public HardwareBase {
public:
    TeensyHardware();
    ~TeensyHardware() override = default;

    // HardwareInterface implementation
    bool init() override;
    void shutdown() override;
    void sendMidiMessage(const MidiMessage& msg) override;
    void setLED(bool on) override;  // Override for PWM brightness control
    void update() override;  // Actively polls hardware GPIO/ADC
    uint32_t getMillis() override;  // Override with Arduino millis()

    // Teensy-specific: PWM LED brightness control
    void setLEDBrightness(uint8_t brightness);

    // Note: readButton(), readRotaryPot(), readSliderPot(), getLED()
    //       inherited from HardwareBase
    // update() reads hardware and updates inherited buttons_/pots_ arrays

private:
    // Pin definitions
    static constexpr int BUTTON_PINS[16] = {
        0, 1, 2, 3, 4, 5, 6, 7,      // B1-B8
        8, 9, 10, 11, 12, 24, 25, 26  // B9-B16
    };

    static constexpr int ROTARY_POT_PINS[4] = {
        A0,  // R1: Mode
        A1,  // R2: Tempo
        A2,  // R3: Pattern
        A3   // R4: Track
    };

    static constexpr int SLIDER_POT_PINS[4] = {
        A4,  // S1
        A5,  // S2
        A6,  // S3
        A7   // S4
    };

    static constexpr int LED_PIN = 13;  // Onboard LED (PWM capable)

    // ADC resolution (Teensy 4.1 supports 10-bit ADC)
    static constexpr int ADC_RESOLUTION = 10;  // 0-1023
    static constexpr int ADC_MAX = (1 << ADC_RESOLUTION) - 1;  // 1023

    // Button debounce delay
    static constexpr uint32_t DEBOUNCE_DELAY_MS = 20;

    // Teensy-specific state (for hardware polling)
    std::array<bool, 16> button_last_states_;
    std::array<uint32_t, 16> button_last_debounce_time_;

    std::array<uint16_t, 4> rotary_pot_raw_values_;  // Raw ADC values with filtering
    std::array<uint16_t, 4> slider_pot_raw_values_;  // Raw ADC values with filtering

    uint8_t led_brightness_;  // 0-255 for PWM (analogWrite)
    uint32_t teensy_start_time_ms_;  // Arduino millis() at init

    // Helper functions
    bool readButtonRaw(int button);
    uint16_t readPotRaw(int pin);
};

} // namespace gruvbok
