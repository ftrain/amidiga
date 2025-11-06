#pragma once

#include "lua_context.h"
#include <memory>
#include <array>
#include <string>

namespace gruvbok {

/**
 * Loads and manages Lua modes
 * One LuaContext per mode
 */
class ModeLoader {
public:
    ModeLoader();

    // Load a specific mode from file
    bool loadMode(int mode_number, const std::string& filepath, int tempo);

    // Get the Lua context for a mode
    LuaContext* getMode(int mode_number);

    // Check if a mode is loaded
    bool isModeLoaded(int mode_number) const;

    // Load all modes from directory
    int loadModesFromDirectory(const std::string& directory, int tempo);

    static constexpr int NUM_MODES = 15;

private:
    std::array<std::unique_ptr<LuaContext>, NUM_MODES> modes_;
};

} // namespace gruvbok
