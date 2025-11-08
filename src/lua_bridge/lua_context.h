#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "../core/event.h"
#include "../hardware/midi_scheduler.h"
#include <string>
#include <vector>

namespace gruvbok {

// Forward declaration
class Engine;

/**
 * Context passed to Lua init() function
 */
struct LuaInitContext {
    int tempo;
    int mode_number;
    int midi_channel;
    int scale_root;      // 0-11 (C-B), controlled by Mode 0
    int scale_type;      // 0-7 (Ionian, Dorian, etc.), controlled by Mode 0
    int velocity_offset; // -64 to +63, controlled by Mode 0
};

/**
 * Wrapper around lua_State for a single mode
 */
class LuaContext {
public:
    LuaContext();
    ~LuaContext();

    // Load Lua script from file
    bool loadScript(const std::string& filepath);

    // Call init(context) function
    bool callInit(const LuaInitContext& context);

    // Call process_event(track, event) function
    // Returns MIDI events to schedule
    std::vector<ScheduledMidiEvent> callProcessEvent(int track, const Event& event);

    // Check if script is loaded and valid
    bool isValid() const { return is_valid_; }

    // Get error message if something failed
    const std::string& getError() const { return error_message_; }

    // Set MIDI channel for this mode
    void setChannel(uint8_t channel);

    // Set Engine instance for LED control
    void setEngine(Engine* engine);

    // Get mode name (from MODE_NAME global variable in Lua)
    std::string getModeName() const;

    // Get slider labels (from SLIDER_LABELS global array in Lua, optional)
    // Returns array of 4 strings (S1-S4 labels)
    std::vector<std::string> getSliderLabels() const;

    // Get Lua state (for testing only)
    lua_State* getState() const { return L_; }

private:
    lua_State* L_;
    bool is_valid_;
    std::string error_message_;
    std::vector<ScheduledMidiEvent> event_buffer_;

    // Helper to check if a function exists
    bool functionExists(const char* name);

    // Helper to handle Lua errors
    void setError(const std::string& error);
};

} // namespace gruvbok
