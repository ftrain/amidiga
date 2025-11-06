#include "lua_api.h"
#include <cstring>

namespace gruvbok {

void LuaAPI::registerAPI(lua_State* L) {
    // Register global functions
    lua_register(L, "note", lua_note);
    lua_register(L, "off", lua_off);
    lua_register(L, "cc", lua_cc);
    lua_register(L, "stopall", lua_stopall);
}

void LuaAPI::setChannel(lua_State* L, uint8_t channel) {
    lua_pushinteger(L, channel);
    lua_setfield(L, LUA_REGISTRYINDEX, CHANNEL_KEY);
}

std::vector<ScheduledMidiEvent>* LuaAPI::getEventBuffer(lua_State* L) {
    lua_getfield(L, LUA_REGISTRYINDEX, EVENT_BUFFER_KEY);
    void* ptr = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return static_cast<std::vector<ScheduledMidiEvent>*>(ptr);
}

void LuaAPI::setEventBuffer(lua_State* L, std::vector<ScheduledMidiEvent>* buffer) {
    lua_pushlightuserdata(L, buffer);
    lua_setfield(L, LUA_REGISTRYINDEX, EVENT_BUFFER_KEY);
}

// ============================================================================
// Lua C API Functions
// ============================================================================

// note(pitch, velocity, [delta])
int LuaAPI::lua_note(lua_State* L) {
    int n = lua_gettop(L);
    if (n < 2) {
        return luaL_error(L, "note() requires at least 2 arguments: pitch, velocity");
    }

    uint8_t pitch = static_cast<uint8_t>(luaL_checkinteger(L, 1));
    uint8_t velocity = static_cast<uint8_t>(luaL_checkinteger(L, 2));
    uint32_t delta = (n >= 3) ? static_cast<uint32_t>(luaL_checkinteger(L, 3)) : 0;

    // Get channel from registry
    lua_getfield(L, LUA_REGISTRYINDEX, CHANNEL_KEY);
    uint8_t channel = static_cast<uint8_t>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    // Get event buffer
    auto* buffer = getEventBuffer(L);
    if (buffer) {
        buffer->push_back(MidiScheduler::noteOn(pitch, velocity, channel, delta));
    }

    return 0;
}

// off(pitch, [delta])
int LuaAPI::lua_off(lua_State* L) {
    int n = lua_gettop(L);
    if (n < 1) {
        return luaL_error(L, "off() requires at least 1 argument: pitch");
    }

    uint8_t pitch = static_cast<uint8_t>(luaL_checkinteger(L, 1));
    uint32_t delta = (n >= 2) ? static_cast<uint32_t>(luaL_checkinteger(L, 2)) : 0;

    // Get channel from registry
    lua_getfield(L, LUA_REGISTRYINDEX, CHANNEL_KEY);
    uint8_t channel = static_cast<uint8_t>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    // Get event buffer
    auto* buffer = getEventBuffer(L);
    if (buffer) {
        buffer->push_back(MidiScheduler::noteOff(pitch, channel, delta));
    }

    return 0;
}

// cc(controller, value, [delta])
int LuaAPI::lua_cc(lua_State* L) {
    int n = lua_gettop(L);
    if (n < 2) {
        return luaL_error(L, "cc() requires at least 2 arguments: controller, value");
    }

    uint8_t controller = static_cast<uint8_t>(luaL_checkinteger(L, 1));
    uint8_t value = static_cast<uint8_t>(luaL_checkinteger(L, 2));
    uint32_t delta = (n >= 3) ? static_cast<uint32_t>(luaL_checkinteger(L, 3)) : 0;

    // Get channel from registry
    lua_getfield(L, LUA_REGISTRYINDEX, CHANNEL_KEY);
    uint8_t channel = static_cast<uint8_t>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    // Get event buffer
    auto* buffer = getEventBuffer(L);
    if (buffer) {
        buffer->push_back(MidiScheduler::controlChange(controller, value, channel, delta));
    }

    return 0;
}

// stopall([delta])
int LuaAPI::lua_stopall(lua_State* L) {
    int n = lua_gettop(L);
    uint32_t delta = (n >= 1) ? static_cast<uint32_t>(luaL_checkinteger(L, 1)) : 0;

    // Get channel from registry
    lua_getfield(L, LUA_REGISTRYINDEX, CHANNEL_KEY);
    uint8_t channel = static_cast<uint8_t>(lua_tointeger(L, -1));
    lua_pop(L, 1);

    // Get event buffer
    auto* buffer = getEventBuffer(L);
    if (buffer) {
        buffer->push_back(MidiScheduler::allNotesOff(channel, delta));
    }

    return 0;
}

} // namespace gruvbok
