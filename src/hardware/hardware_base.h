#pragma once

#include "hardware_interface.h"
#include <array>
#include <cstdint>
#include <algorithm>
#include <chrono>

namespace gruvbok {

/**
 * @brief Base class for hardware implementations providing common functionality
 *
 * This class provides default implementations for button/pot state management,
 * LED control, and timing. Platform-specific implementations (Desktop, Teensy,
 * macOS, iOS) can inherit from this class to avoid code duplication.
 *
 * Derived classes must implement:
 * - init() - Platform-specific initialization
 * - shutdown() - Platform-specific cleanup
 * - sendMidiMessage() - Platform-specific MIDI output
 * - update() - Platform-specific update logic (if needed)
 * - getMillis() - Platform-specific timing (or use default chrono implementation)
 *
 * @note Thread-safety: Not thread-safe. All methods should be called from the same thread.
 */
class HardwareBase : public HardwareInterface {
public:
    HardwareBase();
    virtual ~HardwareBase() override = default;

    // ========================================================================
    // Button Interface (default implementations using internal state)
    // ========================================================================

    /**
     * @brief Read button state
     * @param button Button index (0-15 for B1-B16)
     * @return true if button is currently pressed
     */
    bool readButton(int button) override;

    // ========================================================================
    // Pot Interface (default implementations using internal state)
    // ========================================================================

    /**
     * @brief Read rotary pot value
     * @param pot Rotary pot index (0-3 for R1-R4)
     * @return MIDI value (0-127)
     */
    uint8_t readRotaryPot(int pot) override;

    /**
     * @brief Read slider pot value
     * @param pot Slider pot index (0-3 for S1-S4)
     * @return MIDI value (0-127)
     */
    uint8_t readSliderPot(int pot) override;

    // ========================================================================
    // LED Interface (default implementations using internal state)
    // ========================================================================

    /**
     * @brief Set LED state
     * @param on true to turn LED on, false to turn off
     */
    void setLED(bool on) override;

    /**
     * @brief Get current LED state
     * @return true if LED is on
     */
    bool getLED() const override;

    // ========================================================================
    // Timing Interface (default implementation using std::chrono)
    // ========================================================================

    /**
     * @brief Get milliseconds since initialization
     * @return Milliseconds elapsed since constructor was called
     *
     * @note Default implementation uses std::chrono::steady_clock.
     *       Embedded platforms may override this to use hardware timers.
     */
    uint32_t getMillis() override;

    // ========================================================================
    // Simulation Interface (for desktop/testing platforms)
    // ========================================================================

    /**
     * @brief Simulate button press/release
     * @param button Button index (0-15)
     * @param pressed true to simulate press, false to simulate release
     */
    virtual void simulateButton(int button, bool pressed);

    /**
     * @brief Simulate rotary pot change
     * @param pot Rotary pot index (0-3)
     * @param value MIDI value (0-127)
     */
    virtual void simulateRotaryPot(int pot, uint8_t value);

    /**
     * @brief Simulate slider pot change
     * @param pot Slider pot index (0-3)
     * @param value MIDI value (0-127)
     */
    virtual void simulateSliderPot(int pot, uint8_t value);

protected:
    // ========================================================================
    // Protected State (accessible to derived classes)
    // ========================================================================

    /// Button states (B1-B16)
    std::array<bool, 16> buttons_;

    /// Rotary pot values (R1-R4), MIDI range 0-127
    std::array<uint8_t, 4> rotary_pots_;

    /// Slider pot values (S1-S4), MIDI range 0-127
    std::array<uint8_t, 4> slider_pots_;

    /// LED state
    bool led_state_;

    /// Start time for getMillis() calculation
    std::chrono::steady_clock::time_point start_time_;

    // ========================================================================
    // Protected Utility Methods
    // ========================================================================

    /**
     * @brief Map ADC value to MIDI range (0-127)
     * @param adc_value Raw ADC reading
     * @param adc_max Maximum ADC value (e.g., 1023 for 10-bit, 4095 for 12-bit)
     * @return MIDI value (0-127)
     */
    static uint8_t mapAdcToMidi(uint16_t adc_value, uint16_t adc_max);

    /**
     * @brief Clamp value to MIDI range (0-127)
     * @param value Value to clamp
     * @return Clamped value in range [0, 127]
     */
    static uint8_t clampToMidi(int value);

    /**
     * @brief Validate button index
     * @param button Button index to validate
     * @return true if button index is valid (0-15)
     */
    static bool isValidButton(int button);

    /**
     * @brief Validate pot index
     * @param pot Pot index to validate
     * @return true if pot index is valid (0-3)
     */
    static bool isValidPot(int pot);
};

} // namespace gruvbok
