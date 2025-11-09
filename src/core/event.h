#pragma once

#include "config.h"
#include <cstdint>

namespace gruvbok {

/**
 * Event represents a single step in a sequence.
 * Contains: 1 switch (on/off) + 4 pot values (0-127 each)
 *
 * Memory layout (bit-packed into uint32_t):
 * - Bit 0: Switch (0 or 1)
 * - Bits 1-7: Pot 0 (0-127)
 * - Bits 8-14: Pot 1 (0-127)
 * - Bits 15-21: Pot 2 (0-127)
 * - Bits 22-28: Pot 3 (0-127)
 * - Bits 29-31: Unused
 *
 * Total: 29 bits used, fits in 32-bit integer
 */
class Event {
public:
    Event();
    Event(bool switch_state, uint8_t pot0, uint8_t pot1, uint8_t pot2, uint8_t pot3);

    // Getters
    bool getSwitch() const;
    uint8_t getPot(int index) const;  // index: 0-3

    // Setters
    void setSwitch(bool state);
    void setPot(int index, uint8_t value);  // value: 0-127

    // Clear all data
    void clear();

    // Raw data access (for serialization)
    uint32_t getRawData() const { return data_; }
    void setRawData(uint32_t raw) { data_ = raw; }

private:
    uint32_t data_;

    // Use centralized constants from config.h
    static constexpr uint32_t SWITCH_MASK = config::EVENT_SWITCH_MASK;
    static constexpr int POT0_SHIFT = config::EVENT_POT0_SHIFT;
    static constexpr int POT1_SHIFT = config::EVENT_POT1_SHIFT;
    static constexpr int POT2_SHIFT = config::EVENT_POT2_SHIFT;
    static constexpr int POT3_SHIFT = config::EVENT_POT3_SHIFT;
    static constexpr uint32_t POT_MASK = config::EVENT_POT_MASK;
};

} // namespace gruvbok
