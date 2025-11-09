#include "led_controller.h"

namespace gruvbok {

LEDController::LEDController(HardwareInterface* hardware)
    : hardware_(hardware)
    , pattern_(LEDPattern::TEMPO_BEAT)
    , led_on_(false)
    , brightness_(255)
    , state_start_time_(0)
    , phase_start_time_(0)
    , blink_count_(0) {
}

void LEDController::triggerPattern(LEDPattern pattern, uint8_t brightness) {
    pattern_ = pattern;
    brightness_ = brightness;
    state_start_time_ = hardware_->getMillis();
    phase_start_time_ = state_start_time_;
    blink_count_ = 0;
    led_on_ = true;
    hardware_->setLED(true);
}

void LEDController::triggerPatternByName(const std::string& pattern_name, uint8_t brightness) {
    LEDPattern pattern = LEDPattern::TEMPO_BEAT;

    if (pattern_name == "tempo") pattern = LEDPattern::TEMPO_BEAT;
    else if (pattern_name == "held") pattern = LEDPattern::BUTTON_HELD;
    else if (pattern_name == "saving") pattern = LEDPattern::SAVING;
    else if (pattern_name == "loading") pattern = LEDPattern::LOADING;
    else if (pattern_name == "error") pattern = LEDPattern::ERROR;
    else if (pattern_name == "mirror") pattern = LEDPattern::MIRROR_MODE;

    triggerPattern(pattern, brightness);
}

void LEDController::update() {
    uint32_t current_time = hardware_->getMillis();
    uint32_t pattern_elapsed = current_time - state_start_time_;
    uint32_t phase_elapsed = current_time - phase_start_time_;

    switch (pattern_) {
        case LEDPattern::TEMPO_BEAT:
            // Simple 50ms pulse
            if (led_on_ && phase_elapsed >= LED_TEMPO_DURATION_MS) {
                hardware_->setLED(false);
                led_on_ = false;
            }
            break;

        case LEDPattern::BUTTON_HELD:
            // Fast double-blink: 100ms on, 50ms off, 100ms on, 150ms off (repeat)
            if (pattern_elapsed < 100) {
                if (!led_on_) {
                    hardware_->setLED(true);
                    led_on_ = true;
                }
            } else if (pattern_elapsed < 150) {
                if (led_on_) {
                    hardware_->setLED(false);
                    led_on_ = false;
                }
            } else if (pattern_elapsed < 250) {
                if (!led_on_) {
                    hardware_->setLED(true);
                    led_on_ = true;
                }
            } else if (pattern_elapsed < 400) {
                if (led_on_) {
                    hardware_->setLED(false);
                    led_on_ = false;
                }
            } else {
                // Restart pattern
                state_start_time_ = current_time;
            }
            break;

        case LEDPattern::SAVING:
            // Rapid blinks: 100ms on/off, 5 times total (1 second)
            {
                int cycle = (int)(phase_elapsed / 200);  // Each cycle is 200ms
                if (cycle >= 5) {
                    // Pattern complete, return to tempo
                    pattern_ = LEDPattern::TEMPO_BEAT;
                    hardware_->setLED(false);
                    led_on_ = false;
                } else {
                    bool should_be_on = (phase_elapsed % 200) < 100;
                    if (should_be_on != led_on_) {
                        hardware_->setLED(should_be_on);
                        led_on_ = should_be_on;
                    }
                }
            }
            break;

        case LEDPattern::LOADING:
            // Slow pulse: 1 second on, 1 second off (2 second cycle)
            {
                bool should_be_on = (pattern_elapsed % 2000) < 1000;
                if (should_be_on != led_on_) {
                    hardware_->setLED(should_be_on);
                    led_on_ = should_be_on;
                }
            }
            break;

        case LEDPattern::ERROR:
            // Triple fast blink: 50ms on/off, 3 times (300ms total)
            {
                int cycle = (int)(phase_elapsed / 100);  // Each cycle is 100ms
                if (cycle >= 3) {
                    // Pattern complete, return to tempo
                    pattern_ = LEDPattern::TEMPO_BEAT;
                    hardware_->setLED(false);
                    led_on_ = false;
                } else {
                    bool should_be_on = (phase_elapsed % 100) < 50;
                    if (should_be_on != led_on_) {
                        hardware_->setLED(should_be_on);
                        led_on_ = should_be_on;
                    }
                }
            }
            break;

        case LEDPattern::MIRROR_MODE:
            // Alternating long/short: 200ms on, 100ms off (repeat)
            if (pattern_elapsed < 200) {
                if (!led_on_) {
                    hardware_->setLED(true);
                    led_on_ = true;
                }
            } else if (pattern_elapsed < 300) {
                if (led_on_) {
                    hardware_->setLED(false);
                    led_on_ = false;
                }
            } else {
                // Restart pattern
                state_start_time_ = current_time;
            }
            break;
    }
}

} // namespace gruvbok
