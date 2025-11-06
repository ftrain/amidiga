#include "song.h"
#include <stdexcept>
#include <fstream>

namespace gruvbok {

// ============================================================================
// Mode
// ============================================================================

Mode::Mode() {
    clear();
}

Pattern& Mode::getPattern(int pattern_num) {
    if (pattern_num < 0 || pattern_num >= NUM_PATTERNS) {
        throw std::out_of_range("Pattern number out of range");
    }
    return patterns_[pattern_num];
}

const Pattern& Mode::getPattern(int pattern_num) const {
    if (pattern_num < 0 || pattern_num >= NUM_PATTERNS) {
        throw std::out_of_range("Pattern number out of range");
    }
    return patterns_[pattern_num];
}

void Mode::setPattern(int pattern_num, const Pattern& pattern) {
    if (pattern_num < 0 || pattern_num >= NUM_PATTERNS) {
        throw std::out_of_range("Pattern number out of range");
    }
    patterns_[pattern_num] = pattern;
}

void Mode::clear() {
    for (auto& pattern : patterns_) {
        pattern.clear();
    }
}

// ============================================================================
// Song
// ============================================================================

Song::Song() {
    clear();
}

Mode& Song::getMode(int mode_num) {
    if (mode_num < 0 || mode_num >= NUM_MODES) {
        throw std::out_of_range("Mode number out of range");
    }
    return modes_[mode_num];
}

const Mode& Song::getMode(int mode_num) const {
    if (mode_num < 0 || mode_num >= NUM_MODES) {
        throw std::out_of_range("Mode number out of range");
    }
    return modes_[mode_num];
}

void Song::setMode(int mode_num, const Mode& mode) {
    if (mode_num < 0 || mode_num >= NUM_MODES) {
        throw std::out_of_range("Mode number out of range");
    }
    modes_[mode_num] = mode;
}

void Song::clear() {
    for (auto& mode : modes_) {
        mode.clear();
    }
}

bool Song::save(const std::string& filepath) {
    // TODO: Implement binary serialization
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Write each event's raw data
    for (int m = 0; m < NUM_MODES; ++m) {
        for (int p = 0; p < Mode::NUM_PATTERNS; ++p) {
            for (int t = 0; t < Pattern::NUM_TRACKS; ++t) {
                for (int e = 0; e < Track::NUM_EVENTS; ++e) {
                    uint32_t raw = modes_[m].getPattern(p).getEvent(t, e).getRawData();
                    file.write(reinterpret_cast<const char*>(&raw), sizeof(raw));
                }
            }
        }
    }

    file.close();
    return true;
}

bool Song::load(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Read each event's raw data
    for (int m = 0; m < NUM_MODES; ++m) {
        for (int p = 0; p < Mode::NUM_PATTERNS; ++p) {
            for (int t = 0; t < Pattern::NUM_TRACKS; ++t) {
                for (int e = 0; e < Track::NUM_EVENTS; ++e) {
                    uint32_t raw;
                    file.read(reinterpret_cast<char*>(&raw), sizeof(raw));
                    if (file.fail()) {
                        return false;
                    }
                    modes_[m].getPattern(p).getEvent(t, e).setRawData(raw);
                }
            }
        }
    }

    file.close();
    return true;
}

size_t Song::getMemoryFootprint() {
    // 15 modes × 32 patterns × 8 tracks × 16 events × 4 bytes
    return NUM_MODES * Mode::NUM_PATTERNS * Pattern::NUM_TRACKS * Track::NUM_EVENTS * sizeof(uint32_t);
}

} // namespace gruvbok
