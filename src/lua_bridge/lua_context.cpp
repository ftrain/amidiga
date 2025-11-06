#include "lua_context.h"
#include "lua_api.h"
#include <iostream>

namespace gruvbok {

LuaContext::LuaContext()
    : L_(nullptr), is_valid_(false) {
    L_ = luaL_newstate();
    if (!L_) {
        setError("Failed to create Lua state");
        return;
    }

    luaL_openlibs(L_);
    LuaAPI::registerAPI(L_);
    LuaAPI::setEventBuffer(L_, &event_buffer_);
}

LuaContext::~LuaContext() {
    if (L_) {
        lua_close(L_);
    }
}

bool LuaContext::loadScript(const std::string& filepath) {
    is_valid_ = false;

    if (luaL_dofile(L_, filepath.c_str()) != LUA_OK) {
        setError(std::string("Failed to load script: ") + lua_tostring(L_, -1));
        lua_pop(L_, 1);
        return false;
    }

    // Verify required functions exist
    if (!functionExists("init")) {
        setError("Script missing required function: init()");
        return false;
    }

    if (!functionExists("process_event")) {
        setError("Script missing required function: process_event()");
        return false;
    }

    is_valid_ = true;
    return true;
}

bool LuaContext::callInit(const LuaInitContext& context) {
    if (!is_valid_) {
        return false;
    }

    lua_getglobal(L_, "init");
    if (!lua_isfunction(L_, -1)) {
        setError("init is not a function");
        lua_pop(L_, 1);
        return false;
    }

    // Create context table
    lua_newtable(L_);

    lua_pushinteger(L_, context.tempo);
    lua_setfield(L_, -2, "tempo");

    lua_pushinteger(L_, context.mode_number);
    lua_setfield(L_, -2, "mode_number");

    lua_pushinteger(L_, context.midi_channel);
    lua_setfield(L_, -2, "midi_channel");

    // Call init(context)
    if (lua_pcall(L_, 1, 0, 0) != LUA_OK) {
        setError(std::string("Error calling init(): ") + lua_tostring(L_, -1));
        lua_pop(L_, 1);
        return false;
    }

    return true;
}

std::vector<ScheduledMidiEvent> LuaContext::callProcessEvent(int track, const Event& event) {
    event_buffer_.clear();

    if (!is_valid_) {
        return event_buffer_;
    }

    lua_getglobal(L_, "process_event");
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        return event_buffer_;
    }

    // Push track number
    lua_pushinteger(L_, track);

    // Create event table
    lua_newtable(L_);

    lua_pushboolean(L_, event.getSwitch());
    lua_setfield(L_, -2, "switch");

    // Create pots array
    lua_newtable(L_);
    for (int i = 0; i < 4; ++i) {
        lua_pushinteger(L_, event.getPot(i));
        lua_rawseti(L_, -2, i + 1);  // Lua arrays are 1-indexed
    }
    lua_setfield(L_, -2, "pots");

    // Call process_event(track, event)
    if (lua_pcall(L_, 2, 1, 0) != LUA_OK) {
        std::cerr << "Error calling process_event(): " << lua_tostring(L_, -1) << std::endl;
        lua_pop(L_, 1);
        return event_buffer_;
    }

    // Return value is ignored (events are in buffer)
    lua_pop(L_, 1);

    return event_buffer_;
}

void LuaContext::setChannel(uint8_t channel) {
    LuaAPI::setChannel(L_, channel);
}

bool LuaContext::functionExists(const char* name) {
    lua_getglobal(L_, name);
    bool exists = lua_isfunction(L_, -1);
    lua_pop(L_, 1);
    return exists;
}

void LuaContext::setError(const std::string& error) {
    error_message_ = error;
    is_valid_ = false;
    std::cerr << "LuaContext error: " << error << std::endl;
}

std::string LuaContext::getModeName() const {
    if (!is_valid_) {
        return "Invalid";
    }

    // Try to read MODE_NAME global variable
    lua_getglobal(L_, "MODE_NAME");

    if (lua_isstring(L_, -1)) {
        std::string name = lua_tostring(L_, -1);
        lua_pop(L_, 1);
        return name;
    }

    lua_pop(L_, 1);
    return "Unnamed";  // Default if MODE_NAME not defined
}

} // namespace gruvbok
