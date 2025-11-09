#pragma once

#include "pattern.h"
#include "config.h"
#include <array>
#include <string>

namespace gruvbok {

/**
 * Mode contains 32 Patterns
 * Each mode plays on its own MIDI channel
 */
class Mode {
public:
    Mode();

    Pattern& getPattern(int pattern_num);  // pattern_num: 0-31
    const Pattern& getPattern(int pattern_num) const;

    void setPattern(int pattern_num, const Pattern& pattern);
    void clear();

    static constexpr int NUM_PATTERNS = config::PATTERNS_PER_MODE;

private:
    std::array<Pattern, NUM_PATTERNS> patterns_;
};

/**
 * Song contains 15 Modes (modes 0-14, though mode 0 is boot)
 * This is the top-level data structure
 */
class Song {
public:
    Song();

    Mode& getMode(int mode_num);  // mode_num: 0-14
    const Mode& getMode(int mode_num) const;

    void setMode(int mode_num, const Mode& mode);
    void clear();

    // Persistence (JSON format - human readable, desktop only)
    bool save(const std::string& filepath, const std::string& name = "GRUVBOK Song", int tempo = 120);
    bool load(const std::string& filepath, std::string* out_name = nullptr, int* out_tempo = nullptr);

    // Binary persistence (for flash memory - efficient, static size)
    bool saveBinary(const std::string& filepath);
    bool loadBinary(const std::string& filepath);

    static constexpr int NUM_MODES = config::NUM_MODES;

    // Calculate memory footprint
    static size_t getMemoryFootprint();

private:
    std::array<Mode, NUM_MODES> modes_;
};

} // namespace gruvbok
