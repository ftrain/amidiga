#include "hardware_base.h"

namespace gruvbok {

HardwareBase::HardwareBase()
    : led_state_(false)
    , start_time_(std::chrono::steady_clock::now()) {
    // Initialize all buttons to unpressed
    buttons_.fill(false);

    // Initialize all pots to mid-range (64)
    rotary_pots_.fill(64);
    slider_pots_.fill(64);
}

// ============================================================================
// Button Interface
// ============================================================================

bool HardwareBase::readButton(int button) {
    if (!isValidButton(button)) {
        return false;
    }
    return buttons_[button];
}

// ============================================================================
// Pot Interface
// ============================================================================

uint8_t HardwareBase::readRotaryPot(int pot) {
    if (!isValidPot(pot)) {
        return 0;
    }
    return rotary_pots_[pot];
}

uint8_t HardwareBase::readSliderPot(int pot) {
    if (!isValidPot(pot)) {
        return 0;
    }
    return slider_pots_[pot];
}

// ============================================================================
// LED Interface
// ============================================================================

void HardwareBase::setLED(bool on) {
    led_state_ = on;
}

bool HardwareBase::getLED() const {
    return led_state_;
}

// ============================================================================
// Timing Interface
// ============================================================================

uint32_t HardwareBase::getMillis() {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
    return static_cast<uint32_t>(duration.count());
}

// ============================================================================
// Simulation Interface
// ============================================================================

void HardwareBase::simulateButton(int button, bool pressed) {
    if (isValidButton(button)) {
        buttons_[button] = pressed;
    }
}

void HardwareBase::simulateRotaryPot(int pot, uint8_t value) {
    if (isValidPot(pot)) {
        rotary_pots_[pot] = clampToMidi(value);
    }
}

void HardwareBase::simulateSliderPot(int pot, uint8_t value) {
    if (isValidPot(pot)) {
        slider_pots_[pot] = clampToMidi(value);
    }
}

// ============================================================================
// Protected Utility Methods
// ============================================================================

uint8_t HardwareBase::mapAdcToMidi(uint16_t adc_value, uint16_t adc_max) {
    if (adc_max == 0) {
        return 0;
    }

    // Map ADC range to MIDI range with proper rounding
    uint32_t scaled = (static_cast<uint32_t>(adc_value) * 127) / adc_max;
    return static_cast<uint8_t>(std::min(scaled, 127u));
}

uint8_t HardwareBase::clampToMidi(int value) {
    return static_cast<uint8_t>(std::clamp(value, 0, 127));
}

bool HardwareBase::isValidButton(int button) {
    return button >= 0 && button < 16;
}

bool HardwareBase::isValidPot(int pot) {
    return pot >= 0 && pot < 4;
}

} // namespace gruvbok
