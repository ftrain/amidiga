#pragma once

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include "../hardware/midi_scheduler.h"
#include <vector>

namespace gruvbok {

/**
 * Lua API functions exposed to Lua scripts
 * These are called from Lua modes to generate MIDI events
 */
class LuaAPI {
public:
    // Register all API functions in the Lua state
    static void registerAPI(lua_State* L);

    // Set the current channel for subsequent MIDI calls
    static void setChannel(lua_State* L, uint8_t channel);

    // Get the event buffer where MIDI events are accumulated
    static std::vector<ScheduledMidiEvent>* getEventBuffer(lua_State* L);
    static void setEventBuffer(lua_State* L, std::vector<ScheduledMidiEvent>* buffer);

private:
    // Lua C functions (exposed to Lua)
    static int lua_note(lua_State* L);       // note(pitch, velocity, [delta])
    static int lua_off(lua_State* L);        // off(pitch, [delta])
    static int lua_cc(lua_State* L);         // cc(controller, value, [delta])
    static int lua_stopall(lua_State* L);    // stopall([delta])

    // Registry keys
    static constexpr const char* CHANNEL_KEY = "gruvbok_channel";
    static constexpr const char* EVENT_BUFFER_KEY = "gruvbok_event_buffer";
};

} // namespace gruvbok
