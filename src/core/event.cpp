#include "event.h"
#include <algorithm>

namespace gruvbok {

Event::Event() : data_(0) {}

Event::Event(bool switch_state, uint8_t pot0, uint8_t pot1, uint8_t pot2, uint8_t pot3)
    : data_(0) {
    setSwitch(switch_state);
    setPot(0, pot0);
    setPot(1, pot1);
    setPot(2, pot2);
    setPot(3, pot3);
}

bool Event::getSwitch() const {
    return (data_ & SWITCH_MASK) != 0;
}

uint8_t Event::getPot(int index) const {
    if (index < 0 || index > 3) return 0;

    int shift = 0;
    switch (index) {
        case 0: shift = POT0_SHIFT; break;
        case 1: shift = POT1_SHIFT; break;
        case 2: shift = POT2_SHIFT; break;
        case 3: shift = POT3_SHIFT; break;
    }

    return static_cast<uint8_t>((data_ >> shift) & POT_MASK);
}

void Event::setSwitch(bool state) {
    if (state) {
        data_ |= SWITCH_MASK;
    } else {
        data_ &= ~SWITCH_MASK;
    }
}

void Event::setPot(int index, uint8_t value) {
    if (index < 0 || index > 3) return;

    // Clamp to 0-127
    value = std::min(value, static_cast<uint8_t>(127));

    int shift = 0;
    switch (index) {
        case 0: shift = POT0_SHIFT; break;
        case 1: shift = POT1_SHIFT; break;
        case 2: shift = POT2_SHIFT; break;
        case 3: shift = POT3_SHIFT; break;
    }

    // Clear old value and set new value
    uint32_t mask = POT_MASK << shift;
    data_ = (data_ & ~mask) | (static_cast<uint32_t>(value) << shift);
}

void Event::clear() {
    data_ = 0;
}

} // namespace gruvbok
