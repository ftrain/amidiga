#pragma once

#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace gruvbok {

/**
 * Simple INI file parser for hardware configuration
 *
 * Format:
 *   [section]
 *   key=value  # comment
 *
 * Example:
 *   [buttons]
 *   B1=2  # GPIO pin 2
 *   B2=3  # GPIO pin 3
 *
 * Usage:
 *   ConfigParser config("hardware.ini");
 *   int pin = config.getInt("buttons", "B1", 0);  // Returns 2, or 0 if not found
 */
class ConfigParser {
public:
    /**
     * Load configuration from file
     * @param filepath Path to .ini file
     */
    explicit ConfigParser(const std::string& filepath) {
        load(filepath);
    }

    /**
     * Default constructor (no file loaded)
     */
    ConfigParser() = default;

    /**
     * Load configuration from file
     * @param filepath Path to .ini file
     * @return true if loaded successfully
     */
    bool load(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        std::string current_section;
        std::string line;
        int line_number = 0;

        while (std::getline(file, line)) {
            line_number++;

            // Remove comments
            size_t comment_pos = line.find('#');
            if (comment_pos != std::string::npos) {
                line = line.substr(0, comment_pos);
            }

            // Trim whitespace
            line = trim(line);

            // Skip empty lines
            if (line.empty()) {
                continue;
            }

            // Check for section header [section]
            if (line.front() == '[' && line.back() == ']') {
                current_section = line.substr(1, line.length() - 2);
                current_section = trim(current_section);
                continue;
            }

            // Parse key=value
            size_t equals_pos = line.find('=');
            if (equals_pos == std::string::npos) {
                // Invalid line, skip
                continue;
            }

            std::string key = trim(line.substr(0, equals_pos));
            std::string value = trim(line.substr(equals_pos + 1));

            if (!key.empty() && !current_section.empty()) {
                // Store as "section.key" -> "value"
                data_[current_section + "." + key] = value;
            }
        }

        return true;
    }

    /**
     * Get string value
     * @param section Section name (e.g., "buttons")
     * @param key Key name (e.g., "B1")
     * @param default_value Default if not found
     * @return Value or default
     */
    std::string getString(const std::string& section, const std::string& key,
                         const std::string& default_value = "") const {
        auto it = data_.find(section + "." + key);
        if (it != data_.end()) {
            return it->second;
        }
        return default_value;
    }

    /**
     * Get integer value
     * @param section Section name
     * @param key Key name
     * @param default_value Default if not found or invalid
     * @return Parsed integer or default
     */
    int getInt(const std::string& section, const std::string& key, int default_value = 0) const {
        std::string value = getString(section, key);
        if (value.empty()) {
            return default_value;
        }

        try {
            // Handle hexadecimal (0x prefix)
            if (value.size() > 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
                return std::stoi(value, nullptr, 16);
            }
            return std::stoi(value);
        } catch (...) {
            return default_value;
        }
    }

    /**
     * Get boolean value (true/false, yes/no, 1/0)
     * @param section Section name
     * @param key Key name
     * @param default_value Default if not found
     * @return Parsed boolean or default
     */
    bool getBool(const std::string& section, const std::string& key, bool default_value = false) const {
        std::string value = getString(section, key);
        if (value.empty()) {
            return default_value;
        }

        // Convert to lowercase for comparison
        std::string lower = value;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (lower == "true" || lower == "yes" || lower == "1" || lower == "on") {
            return true;
        } else if (lower == "false" || lower == "no" || lower == "0" || lower == "off") {
            return false;
        }

        return default_value;
    }

    /**
     * Check if a key exists
     * @param section Section name
     * @param key Key name
     * @return true if key exists
     */
    bool hasKey(const std::string& section, const std::string& key) const {
        return data_.find(section + "." + key) != data_.end();
    }

    /**
     * Check if parser has any data loaded
     * @return true if data loaded
     */
    bool isLoaded() const {
        return !data_.empty();
    }

    /**
     * Get all keys in a section
     * @param section Section name
     * @return Vector of key names (without section prefix)
     */
    std::vector<std::string> getKeys(const std::string& section) const {
        std::vector<std::string> keys;
        std::string prefix = section + ".";

        for (const auto& pair : data_) {
            if (pair.first.find(prefix) == 0) {
                keys.push_back(pair.first.substr(prefix.length()));
            }
        }

        return keys;
    }

private:
    std::map<std::string, std::string> data_;

    /**
     * Trim whitespace from both ends of string
     */
    static std::string trim(const std::string& str) {
        size_t start = 0;
        size_t end = str.length();

        while (start < end && std::isspace(static_cast<unsigned char>(str[start]))) {
            start++;
        }

        while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
            end--;
        }

        return str.substr(start, end - start);
    }
};

} // namespace gruvbok
