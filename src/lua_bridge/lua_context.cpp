#include "lua_context.h"
#include "lua_api.h"
#include <iostream>

// ============================================================================
// Lua Version Compatibility Checks
// ============================================================================

// Ensure LUA_OK is defined (Lua 5.4 has it, Lua 5.1 doesn't)
#ifndef LUA_OK
    #warning "LUA_OK not defined - add -DLUA_OK=0 to build flags for Lua 5.1 compatibility"
    #define LUA_OK 0
#endif

// Detect Lua version and warn if unexpected
#if defined(LUA_VERSION_NUM)
    #if LUA_VERSION_NUM == 501
        #pragma message("Compiling with Lua 5.1 (e.g., LuaArduino)")
    #elif LUA_VERSION_NUM == 504
        #pragma message("Compiling with Lua 5.4")
    #else
        #warning "Unknown Lua version - expected 5.1 or 5.4"
    #endif
#else
    #warning "Could not detect Lua version"
#endif

namespace gruvbok {

LuaContext::LuaContext()
    : L_(nullptr), is_valid_(false) {
    L_ = luaL_newstate();
    if (!L_) {
        setError("Failed to create Lua state");
        return;
    }

#ifdef NO_EXCEPTIONS
    // For embedded systems (Teensy): Load only essential Lua libraries
    // to minimize memory footprint
    luaL_requiref(L_, "_G", luaopen_base, 1);       // Basic functions
    luaL_requiref(L_, LUA_TABLIBNAME, luaopen_table, 1);  // table.*
    luaL_requiref(L_, LUA_STRLIBNAME, luaopen_string, 1); // string.*
    luaL_requiref(L_, LUA_MATHLIBNAME, luaopen_math, 1);  // math.*
    lua_pop(L_, 4);  // Remove libs from stack

    // Omitted for embedded:
    // - io (liolib): File I/O not needed
    // - os (loslib): OS functions not available on Teensy
    // - package/loadlib: Dynamic loading not needed
    // - debug: Not needed for production
    // - coroutine: Not used in our modes
#else
    // Desktop: Load all standard libraries
    luaL_openlibs(L_);
#endif

    LuaAPI::registerAPI(L_);
    LuaAPI::setEventBuffer(L_, &event_buffer_);

    // Configure Lua GC to minimize pauses during real-time playback
    // setpause(200): GC runs when memory is 200% of previous collection (less frequent, larger pauses)
    // setstep(200): GC performs more work per step (reduces total pause count)
    // This balances memory usage vs. pause frequency for real-time MIDI timing
    lua_gc(L_, LUA_GCSETPAUSE, 200);
    lua_gc(L_, LUA_GCSETSTEPMUL, 200);

    // Context is valid (Lua state created successfully)
    // is_valid_ will be set to false if script loading fails
    is_valid_ = true;
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

    lua_pushinteger(L_, context.scale_root);
    lua_setfield(L_, -2, "scale_root");

    lua_pushinteger(L_, context.scale_type);
    lua_setfield(L_, -2, "scale_type");

    lua_pushinteger(L_, context.velocity_offset);
    lua_setfield(L_, -2, "velocity_offset");

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

void LuaContext::setEngine(Engine* engine) {
    LuaAPI::setEngine(L_, engine);
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
    if (!is_valid_ || !L_) {
        return "Invalid";
    }

    // Try to read MODE_NAME global variable
    lua_getglobal(L_, "MODE_NAME");

    if (lua_isstring(L_, -1)) {
        const char* str = lua_tostring(L_, -1);
        std::string name = str ? str : "Unnamed";
        lua_pop(L_, 1);
        return name;
    }

    lua_pop(L_, 1);
    return "Unnamed";  // Default if MODE_NAME not defined
}

std::vector<std::string> LuaContext::getSliderLabels() const {
    std::vector<std::string> labels = {"S1", "S2", "S3", "S4"};  // Defaults

    if (!is_valid_ || !L_) {
        return labels;
    }

    // Try to read SLIDER_LABELS global table
    lua_getglobal(L_, "SLIDER_LABELS");

    if (lua_istable(L_, -1)) {
        // Read up to 4 labels from the table
        for (int i = 0; i < 4; ++i) {
            lua_rawgeti(L_, -1, i + 1);  // Lua arrays are 1-indexed
            if (lua_isstring(L_, -1)) {
                const char* str = lua_tostring(L_, -1);
                if (str) {
                    labels[i] = str;
                }
            }
            lua_pop(L_, 1);
        }
    }

    lua_pop(L_, 1);
    return labels;
}

} // namespace gruvbok
