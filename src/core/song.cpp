#include "song.h"
#include <stdexcept>
#include <fstream>
#include "../../external/nlohmann/json.hpp"

using json = nlohmann::json;

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
    try {
        json j;
        j["version"] = "1.0";
        j["name"] = "GRUVBOK Song";
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
}

bool Song::load(const std::string& filepath) {
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
}

size_t Song::getMemoryFootprint() {
    // 15 modes × 32 patterns × 8 tracks × 16 events × 4 bytes
    return NUM_MODES * Mode::NUM_PATTERNS * Pattern::NUM_TRACKS * Track::NUM_EVENTS * sizeof(uint32_t);
}

} // namespace gruvbok
