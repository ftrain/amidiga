#pragma once

#include <cstdint>
#include <algorithm>

namespace gruvbok {

/**
 * Common utility functions for hardware implementations
 * Provides standardized debouncing, value mapping, and validation
 */
class HardwareUtils {
public:
    /**
     * Map ADC value to MIDI range (0-127)
     * @param adc_value Raw ADC reading
     * @param adc_max Maximum ADC value (e.g., 1023 for 10-bit)
     * @return MIDI value 0-127
     */
    static uint8_t mapAdcToMidi(uint16_t adc_value, uint16_t adc_max) {
        uint32_t midi_value = (static_cast<uint32_t>(adc_value) * 127) / adc_max;
        return static_cast<uint8_t>(std::min(midi_value, static_cast<uint32_t>(127)));
    }

    /**
     * Apply hysteresis filter to reduce pot jitter
     * Value only changes if it differs by more than threshold
     * @param new_value New reading from pot
     * @param old_value Previous value
     * @param threshold Minimum change to register (default 2)
     * @return Filtered value
     */
    static uint8_t applyHysteresis(uint8_t new_value, uint8_t old_value, uint8_t threshold = 2) {
        int diff = static_cast<int>(new_value) - static_cast<int>(old_value);
        if (diff > threshold || diff < -threshold) {
            return new_value;
        }
        return old_value;
    }

    /**
     * Apply IIR (Infinite Impulse Response) filter to smooth pot readings
     * @param new_value New raw reading
     * @param old_value Previous filtered value
     * @param alpha Filter coefficient (0-256, lower = more smoothing)
     * @return Filtered value
     */
    static uint16_t applyIIRFilter(uint16_t new_value, uint16_t old_value, uint16_t alpha = 64) {
        // filtered = (alpha * new + (256-alpha) * old) / 256
        uint32_t result = (alpha * new_value + (256 - alpha) * old_value) / 256;
        return static_cast<uint16_t>(result);
    }

    /**
     * Validate button index
     */
    static bool isValidButton(int button) {
        return button >= 0 && button < 16;
    }

    /**
     * Validate pot index
     */
    static bool isValidPot(int pot) {
        return pot >= 0 && pot < 4;
    }

    /**
     * Clamp value to MIDI range (0-127)
     */
    static uint8_t clampToMidi(int value) {
        if (value < 0) return 0;
        if (value > 127) return 127;
        return static_cast<uint8_t>(value);
    }

    /**
     * Button debounce state tracker
     */
    struct ButtonDebounce {
        bool current_state = false;
        bool last_reading = false;
        uint32_t last_change_time = 0;
        static constexpr uint32_t DEBOUNCE_DELAY_MS = 20;

        /**
         * Update debounce state with new reading
         * @param reading Current button reading
         * @param current_time Current timestamp in milliseconds
         * @return True if state is stable and should be used
         */
        bool update(bool reading, uint32_t current_time) {
            if (reading != last_reading) {
                last_change_time = current_time;
                last_reading = reading;
                return false;  // State not stable yet
            }

            if ((current_time - last_change_time) > DEBOUNCE_DELAY_MS) {
                current_state = reading;
                return true;  // State is stable
            }

            return false;  // Still debouncing
        }

        bool getState() const { return current_state; }
    };
};

} // namespace gruvbok
