#pragma once

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

    static constexpr uint32_t SWITCH_MASK = 0x00000001;
    static constexpr int POT0_SHIFT = 1;
    static constexpr int POT1_SHIFT = 8;
    static constexpr int POT2_SHIFT = 15;
    static constexpr int POT3_SHIFT = 22;
    static constexpr uint32_t POT_MASK = 0x7F;  // 7 bits

    // Compile-time validation of bit packing layout
    static_assert(POT0_SHIFT + 7 <= 32, "POT0 would overflow uint32_t");
    static_assert(POT1_SHIFT + 7 <= 32, "POT1 would overflow uint32_t");
    static_assert(POT2_SHIFT + 7 <= 32, "POT2 would overflow uint32_t");
    static_assert(POT3_SHIFT + 7 <= 32, "POT3 would overflow uint32_t");
};

} // namespace gruvbok
