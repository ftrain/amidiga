#include "song.h"
#ifndef NO_EXCEPTIONS
#include <stdexcept>
#include <fstream>
#include "../../external/nlohmann/json.hpp"
#endif
#include <algorithm>

#ifndef NO_EXCEPTIONS
using json = nlohmann::json;
#endif

namespace gruvbok {

// ============================================================================
// Mode
// ============================================================================

Mode::Mode() {
    clear();
}

Pattern& Mode::getPattern(int pattern_num) {
#ifndef NO_EXCEPTIONS
    if (pattern_num < 0 || pattern_num >= NUM_PATTERNS) {
        throw std::out_of_range("Pattern number out of range");
    }
#endif
    // Clamp to valid range for embedded builds (defensive programming)
    pattern_num = std::max(0, std::min(pattern_num, NUM_PATTERNS - 1));
    return patterns_[pattern_num];
}

const Pattern& Mode::getPattern(int pattern_num) const {
#ifndef NO_EXCEPTIONS
    if (pattern_num < 0 || pattern_num >= NUM_PATTERNS) {
        throw std::out_of_range("Pattern number out of range");
    }
#endif
    // Clamp to valid range for embedded builds (defensive programming)
    pattern_num = std::max(0, std::min(pattern_num, NUM_PATTERNS - 1));
    return patterns_[pattern_num];
}

void Mode::setPattern(int pattern_num, const Pattern& pattern) {
#ifndef NO_EXCEPTIONS
    if (pattern_num < 0 || pattern_num >= NUM_PATTERNS) {
        throw std::out_of_range("Pattern number out of range");
    }
#endif
    // Clamp to valid range for embedded builds (defensive programming)
    pattern_num = std::max(0, std::min(pattern_num, NUM_PATTERNS - 1));
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
#ifndef NO_EXCEPTIONS
    if (mode_num < 0 || mode_num >= NUM_MODES) {
        throw std::out_of_range("Mode number out of range");
    }
#endif
    // Clamp to valid range for embedded builds (defensive programming)
    mode_num = std::max(0, std::min(mode_num, NUM_MODES - 1));
    return modes_[mode_num];
}

const Mode& Song::getMode(int mode_num) const {
#ifndef NO_EXCEPTIONS
    if (mode_num < 0 || mode_num >= NUM_MODES) {
        throw std::out_of_range("Mode number out of range");
    }
#endif
    // Clamp to valid range for embedded builds (defensive programming)
    mode_num = std::max(0, std::min(mode_num, NUM_MODES - 1));
    return modes_[mode_num];
}

void Song::setMode(int mode_num, const Mode& mode) {
#ifndef NO_EXCEPTIONS
    if (mode_num < 0 || mode_num >= NUM_MODES) {
        throw std::out_of_range("Mode number out of range");
    }
#endif
    // Clamp to valid range for embedded builds (defensive programming)
    mode_num = std::max(0, std::min(mode_num, NUM_MODES - 1));
    modes_[mode_num] = mode;
}

void Song::clear() {
    for (auto& mode : modes_) {
        mode.clear();
    }
}

bool Song::save(const std::string& filepath, const std::string& name, int tempo) {
#ifdef NO_EXCEPTIONS
    // Save/load not available in NO_EXCEPTIONS builds (Teensy will use SD card binary format)
    (void)filepath;
    (void)name;
    (void)tempo;
    return false;
#else
    try {
        json j;
        j["version"] = "1.0";
        j["name"] = name;
        j["tempo"] = tempo;
        j["events"] = json::array();

        // Iterate through all events and save only non-empty ones (sparse format)
        for (int mode_num = 0; mode_num < NUM_MODES; ++mode_num) {
            const Mode& mode = modes_[mode_num];
            for (int pattern_num = 0; pattern_num < Mode::NUM_PATTERNS; ++pattern_num) {
                const Pattern& pattern = mode.getPattern(pattern_num);
                for (int track_num = 0; track_num < Pattern::NUM_TRACKS; ++track_num) {
                    for (int step = 0; step < Track::NUM_EVENTS; ++step) {
                        const Event& evt = pattern.getEvent(track_num, step);

                        // Only save events with switch on
                        if (evt.getSwitch()) {
                            json event_json;
                            event_json["mode"] = mode_num;
                            event_json["pattern"] = pattern_num;
                            event_json["track"] = track_num;
                            event_json["step"] = step;
                            event_json["switch"] = true;
                            event_json["pots"] = {
                                evt.getPot(0),
                                evt.getPot(1),
                                evt.getPot(2),
                                evt.getPot(3)
                            };
                            j["events"].push_back(event_json);
                        }
                    }
                }
            }
        }

        // Write to file with indentation
        std::ofstream file(filepath);
        if (!file.is_open()) {
            return false;
        }
        file << j.dump(2);  // Pretty print with 2-space indent
        file.close();
        return true;

    } catch (const std::exception& e) {
        return false;
    }
#endif
}

bool Song::load(const std::string& filepath, std::string* out_name, int* out_tempo) {
#ifdef NO_EXCEPTIONS
    // Save/load not available in NO_EXCEPTIONS builds (Teensy will use SD card binary format)
    (void)filepath;
    (void)out_name;
    (void)out_tempo;
    return false;
#else
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        json j;
        file >> j;
        file.close();

        // Validate version
        if (!j.contains("version") || j["version"] != "1.0") {
            return false;
        }

        // Load metadata (optional)
        if (out_name && j.contains("name")) {
            *out_name = j["name"];
        }
        if (out_tempo && j.contains("tempo")) {
            *out_tempo = j["tempo"];
        }

        // Clear existing song data
        clear();

        // Load events (sparse format)
        if (j.contains("events") && j["events"].is_array()) {
            for (const auto& event_json : j["events"]) {
                // Validate event structure
                if (!event_json.contains("mode") || !event_json.contains("pattern") ||
                    !event_json.contains("track") || !event_json.contains("step") ||
                    !event_json.contains("switch") || !event_json.contains("pots")) {
                    continue;  // Skip malformed events
                }

                int mode_num = event_json["mode"];
                int pattern_num = event_json["pattern"];
                int track_num = event_json["track"];
                int step = event_json["step"];

                // Validate ranges
                if (mode_num < 0 || mode_num >= NUM_MODES ||
                    pattern_num < 0 || pattern_num >= Mode::NUM_PATTERNS ||
                    track_num < 0 || track_num >= Pattern::NUM_TRACKS ||
                    step < 0 || step >= Track::NUM_EVENTS) {
                    continue;  // Skip out-of-range events
                }

                // Get event reference
                Event& evt = modes_[mode_num].getPattern(pattern_num).getEvent(track_num, step);

                // Set switch
                evt.setSwitch(event_json["switch"]);

                // Set pots
                if (event_json["pots"].is_array() && event_json["pots"].size() == 4) {
                    evt.setPot(0, event_json["pots"][0]);
                    evt.setPot(1, event_json["pots"][1]);
                    evt.setPot(2, event_json["pots"][2]);
                    evt.setPot(3, event_json["pots"][3]);
                }
            }
        }

        return true;

    } catch (const std::exception& e) {
        return false;
    }
#endif
}

size_t Song::getMemoryFootprint() {
    // 15 modes × 32 patterns × 8 tracks × 16 events × 4 bytes
    return NUM_MODES * Mode::NUM_PATTERNS * Pattern::NUM_TRACKS * Track::NUM_EVENTS * sizeof(uint32_t);
}

bool Song::saveBinary(const std::string& filepath) {
#ifdef NO_EXCEPTIONS
    // For embedded: Write to flash memory region
    // This will be implemented in Teensy-specific code
    (void)filepath;
    return false;
#else
    try {
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        // Write magic number and version
        const uint32_t magic = 0x47525642;  // "GRVB" in ASCII
        const uint32_t version = 1;
        file.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));

        // Write raw event data for all modes
        // The Song/Mode/Pattern/Track hierarchy is just nested arrays, so we can write directly
        for (int mode_num = 0; mode_num < NUM_MODES; ++mode_num) {
            const Mode& mode = modes_[mode_num];
            for (int pattern_num = 0; pattern_num < Mode::NUM_PATTERNS; ++pattern_num) {
                const Pattern& pattern = mode.getPattern(pattern_num);
                for (int track_num = 0; track_num < Pattern::NUM_TRACKS; ++track_num) {
                    for (int step = 0; step < Track::NUM_EVENTS; ++step) {
                        const Event& evt = pattern.getEvent(track_num, step);
                        uint32_t packed = evt.getRawData();  // Get bit-packed representation
                        file.write(reinterpret_cast<const char*>(&packed), sizeof(packed));
                    }
                }
            }
        }

        file.close();
        return true;

    } catch (const std::exception& e) {
        return false;
    }
#endif
}

bool Song::loadBinary(const std::string& filepath) {
#ifdef NO_EXCEPTIONS
    // For embedded: Read from flash memory region
    // This will be implemented in Teensy-specific code
    (void)filepath;
    return false;
#else
    try {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        // Read and validate magic number
        uint32_t magic = 0;
        file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        if (magic != 0x47525642) {  // "GRVB"
            return false;
        }

        // Read version
        uint32_t version = 0;
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        if (version != 1) {
            return false;
        }

        // Read raw event data for all modes
        for (int mode_num = 0; mode_num < NUM_MODES; ++mode_num) {
            Mode& mode = modes_[mode_num];
            for (int pattern_num = 0; pattern_num < Mode::NUM_PATTERNS; ++pattern_num) {
                Pattern& pattern = mode.getPattern(pattern_num);
                for (int track_num = 0; track_num < Pattern::NUM_TRACKS; ++track_num) {
                    for (int step = 0; step < Track::NUM_EVENTS; ++step) {
                        uint32_t packed = 0;
                        file.read(reinterpret_cast<char*>(&packed), sizeof(packed));
                        Event& evt = pattern.getEvent(track_num, step);
                        evt.setRawData(packed);
                    }
                }
            }
        }

        file.close();
        return true;

    } catch (const std::exception& e) {
        return false;
    }
#endif
}

} // namespace gruvbok
