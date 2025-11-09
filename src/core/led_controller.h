#pragma once

#include "../hardware/hardware_interface.h"
#include <cstdint>
#include <string>

namespace gruvbok {

/**
 * LED pattern types for visual feedback
 */
enum class LEDPattern {
    TEMPO_BEAT,     // Simple pulse on each beat (50ms)
    BUTTON_HELD,    // Fast double-blink pattern
    SAVING,         // Rapid blinks (5 times)
    LOADING,        // Slow pulse (1s on/off)
    ERROR,          // Triple fast blink
    MIRROR_MODE     // Alternating long/short blinks
};

/**
 * LED Controller - Manages LED patterns for visual feedback
 *
 * Separates LED pattern management from the main Engine class.
 * Provides different visual feedback patterns for various system states.
 */
class LEDController {
public:
    explicit LEDController(HardwareInterface* hardware);

    /**
     * Trigger an LED pattern
     * @param pattern Pattern to display
     * @param brightness LED brightness (0-255)
     */
    void triggerPattern(LEDPattern pattern, uint8_t brightness = 255);

    /**
     * Trigger LED pattern by name (for Lua API)
     * @param pattern_name Pattern name ("tempo", "held", "saving", etc.)
     * @param brightness LED brightness (0-255)
     */
    void triggerPatternByName(const std::string& pattern_name, uint8_t brightness = 255);

    /**
     * Update LED state (call frequently in main loop)
     */
    void update();

    /**
     * Get current pattern
     */
    LEDPattern getCurrentPattern() const { return pattern_; }

private:
    HardwareInterface* hardware_;
    LEDPattern pattern_;
    bool led_on_;
    uint8_t brightness_;
    uint32_t state_start_time_;
    uint32_t phase_start_time_;
    int blink_count_;

    static constexpr uint32_t LED_TEMPO_DURATION_MS = 50;
};

} // namespace gruvbok
