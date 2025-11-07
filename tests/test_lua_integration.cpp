/**
 * Lua Integration Tests
 *
 * Tests the Lua bridge layer and compatibility:
 * - Lua state creation/destruction
 * - API registration (note, off, cc, stopall, led)
 * - Mode script loading
 * - Lua 5.1 vs 5.4 compatibility
 * - Event processing and MIDI generation
 */

#include "../src/lua_bridge/lua_context.h"
#include "../src/lua_bridge/lua_api.h"
#include "../src/core/event.h"
#include <iostream>
#include <cassert>
#include <fstream>

using namespace gruvbok;

// Simple test framework
int test_count = 0;
int pass_count = 0;
int fail_count = 0;

#define TEST(name) \
    void test_##name(); \
    void run_test_##name() { \
        std::cout << "Running test: " << #name << "... "; \
        try { \
            test_##name(); \
            std::cout << "PASS" << std::endl; \
            pass_count++; \
        } catch (const std::exception& e) { \
            std::cout << "FAIL: " << e.what() << std::endl; \
            fail_count++; \
        } \
        test_count++; \
    } \
    void test_##name()

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        throw std::runtime_error(std::string("Expected ") + #a + " == " + #b + \
                                 ", got " + std::to_string(a) + " != " + std::to_string(b)); \
    }

#define ASSERT_TRUE(expr) \
    if (!(expr)) { \
        throw std::runtime_error(std::string("Expected ") + #expr + " to be true"); \
    }

#define ASSERT_FALSE(expr) \
    if (expr) { \
        throw std::runtime_error(std::string("Expected ") + #expr + " to be false"); \
    }

// Helper: Create a temporary test Lua script
std::string createTempLuaScript(const std::string& content) {
    static int counter = 0;
    std::string filename = "/tmp/gruvbok_test_" + std::to_string(counter++) + ".lua";
    std::ofstream file(filename);
    file << content;
    file.close();
    return filename;
}

// ============================================================================
// Lua Version Compatibility Tests
// ============================================================================

TEST(lua_version_detected) {
    LuaContext ctx;
    lua_State* L = ctx.getState();

    // Check Lua version (should be 5.1 or 5.4)
    lua_getglobal(L, "_VERSION");
    const char* version = lua_tostring(L, -1);
    lua_pop(L, 1);

    std::cout << "\n    Detected: " << version << " ";

    // Verify it's a known version
    std::string ver_str(version);
    ASSERT_TRUE(ver_str.find("Lua 5.1") != std::string::npos ||
                ver_str.find("Lua 5.4") != std::string::npos);
}

TEST(lua_ok_constant_defined) {
    // Verify LUA_OK is defined (either natively in 5.4 or via -DLUA_OK=0 for 5.1)
    #ifndef LUA_OK
        throw std::runtime_error("LUA_OK not defined! Add -DLUA_OK=0 to build flags for Lua 5.1");
    #endif

    ASSERT_EQ(LUA_OK, 0);
}

// ============================================================================
// Lua Context Tests
// ============================================================================

TEST(lua_context_creation) {
    LuaContext ctx;
    ASSERT_TRUE(ctx.isValid());
    ASSERT_TRUE(ctx.getState() != nullptr);
}

TEST(lua_api_registration) {
    LuaContext ctx;
    lua_State* L = ctx.getState();

    // Verify all API functions are registered as globals
    lua_getglobal(L, "note");
    ASSERT_TRUE(lua_isfunction(L, -1));
    lua_pop(L, 1);

    lua_getglobal(L, "off");
    ASSERT_TRUE(lua_isfunction(L, -1));
    lua_pop(L, 1);

    lua_getglobal(L, "cc");
    ASSERT_TRUE(lua_isfunction(L, -1));
    lua_pop(L, 1);

    lua_getglobal(L, "stopall");
    ASSERT_TRUE(lua_isfunction(L, -1));
    lua_pop(L, 1);

    lua_getglobal(L, "led");
    ASSERT_TRUE(lua_isfunction(L, -1));
    lua_pop(L, 1);
}

// ============================================================================
// Script Loading Tests
// ============================================================================

TEST(load_simple_script) {
    std::string script = createTempLuaScript(R"(
        MODE_NAME = "Test Mode"

        function init(context)
            -- Do nothing
        end

        function process_event(track, event)
            -- Return nothing
        end
    )");

    LuaContext ctx;
    ASSERT_TRUE(ctx.loadScript(script));
    ASSERT_TRUE(ctx.isValid());
    ASSERT_EQ(ctx.getModeName(), "Test Mode");
}

TEST(reject_script_without_init) {
    std::string script = createTempLuaScript(R"(
        function process_event(track, event)
            -- Missing init()
        end
    )");

    LuaContext ctx;
    ASSERT_FALSE(ctx.loadScript(script));
    ASSERT_FALSE(ctx.isValid());
}

TEST(reject_script_without_process_event) {
    std::string script = createTempLuaScript(R"(
        function init(context)
            -- Missing process_event()
        end
    )");

    LuaContext ctx;
    ASSERT_FALSE(ctx.loadScript(script));
    ASSERT_FALSE(ctx.isValid());
}

TEST(reject_script_with_syntax_error) {
    std::string script = createTempLuaScript(R"(
        function init(context)
            -- Syntax error: missing end

        function process_event(track, event)
        end
    )");

    LuaContext ctx;
    ASSERT_FALSE(ctx.loadScript(script));
    ASSERT_FALSE(ctx.isValid());
}

// ============================================================================
// Init Context Tests
// ============================================================================

TEST(call_init_with_context) {
    std::string script = createTempLuaScript(R"(
        received_tempo = 0
        received_mode = -1
        received_channel = -1

        function init(context)
            received_tempo = context.tempo
            received_mode = context.mode_number
            received_channel = context.midi_channel
        end

        function process_event(track, event)
        end
    )");

    LuaContext ctx;
    ASSERT_TRUE(ctx.loadScript(script));

    LuaInitContext init_ctx;
    init_ctx.tempo = 120;
    init_ctx.mode_number = 5;
    init_ctx.midi_channel = 5;

    ASSERT_TRUE(ctx.callInit(init_ctx));

    // Verify Lua received the context
    lua_State* L = ctx.getState();
    lua_getglobal(L, "received_tempo");
    ASSERT_EQ(lua_tointeger(L, -1), 120);
    lua_pop(L, 1);

    lua_getglobal(L, "received_mode");
    ASSERT_EQ(lua_tointeger(L, -1), 5);
    lua_pop(L, 1);

    lua_getglobal(L, "received_channel");
    ASSERT_EQ(lua_tointeger(L, -1), 5);
    lua_pop(L, 1);
}

// ============================================================================
// Event Processing Tests
// ============================================================================

TEST(process_event_with_switch_on) {
    std::string script = createTempLuaScript(R"(
        function init(context)
        end

        function process_event(track, event)
            if event.switch then
                note(60, 100, 0)
                off(60, 100)
            end
        end
    )");

    LuaContext ctx;
    ctx.setChannel(0);
    ASSERT_TRUE(ctx.loadScript(script));

    LuaInitContext init_ctx = {120, 0, 0};
    ASSERT_TRUE(ctx.callInit(init_ctx));

    // Create event with switch ON
    Event evt;
    evt.setSwitch(true);
    evt.setPot(0, 64);

    auto midi_events = ctx.callProcessEvent(0, evt);

    // Should generate 2 MIDI events: note on + note off
    ASSERT_EQ(midi_events.size(), 2);
    ASSERT_EQ(midi_events[0].message[0], 0x90); // Note On, channel 0
    ASSERT_EQ(midi_events[0].message[1], 60);   // Middle C
    ASSERT_EQ(midi_events[0].message[2], 100);  // Velocity
    ASSERT_EQ(midi_events[1].message[0], 0x80); // Note Off, channel 0
}

TEST(process_event_with_switch_off) {
    std::string script = createTempLuaScript(R"(
        function init(context)
        end

        function process_event(track, event)
            if event.switch then
                note(60, 100)
            end
        end
    )");

    LuaContext ctx;
    ctx.setChannel(0);
    ASSERT_TRUE(ctx.loadScript(script));

    LuaInitContext init_ctx = {120, 0, 0};
    ASSERT_TRUE(ctx.callInit(init_ctx));

    // Create event with switch OFF
    Event evt;
    evt.setSwitch(false);

    auto midi_events = ctx.callProcessEvent(0, evt);

    // Should generate 0 MIDI events
    ASSERT_EQ(midi_events.size(), 0);
}

TEST(process_event_reads_pot_values) {
    std::string script = createTempLuaScript(R"(
        function init(context)
        end

        function process_event(track, event)
            if event.switch then
                -- Use pot values for pitch, velocity
                local pitch = event.pots[1]
                local velocity = event.pots[2]
                note(pitch, velocity)
            end
        end
    )");

    LuaContext ctx;
    ctx.setChannel(0);
    ASSERT_TRUE(ctx.loadScript(script));

    LuaInitContext init_ctx = {120, 0, 0};
    ASSERT_TRUE(ctx.callInit(init_ctx));

    // Create event with custom pot values
    Event evt;
    evt.setSwitch(true);
    evt.setPot(0, 72);  // Pots are 1-indexed in Lua (pots[1])
    evt.setPot(1, 110);

    auto midi_events = ctx.callProcessEvent(0, evt);

    ASSERT_EQ(midi_events.size(), 1);
    ASSERT_EQ(midi_events[0].message[1], 72);  // Pitch from pot 1
    ASSERT_EQ(midi_events[0].message[2], 110); // Velocity from pot 2
}

// ============================================================================
// MIDI Generation Tests
// ============================================================================

TEST(generate_control_change) {
    std::string script = createTempLuaScript(R"(
        function init(context)
        end

        function process_event(track, event)
            if event.switch then
                cc(74, event.pots[1])  -- Filter cutoff
            end
        end
    )");

    LuaContext ctx;
    ctx.setChannel(0);
    ASSERT_TRUE(ctx.loadScript(script));

    LuaInitContext init_ctx = {120, 0, 0};
    ASSERT_TRUE(ctx.callInit(init_ctx));

    Event evt;
    evt.setSwitch(true);
    evt.setPot(0, 64);

    auto midi_events = ctx.callProcessEvent(0, evt);

    ASSERT_EQ(midi_events.size(), 1);
    ASSERT_EQ(midi_events[0].message[0], 0xB0); // CC, channel 0
    ASSERT_EQ(midi_events[0].message[1], 74);   // Controller number
    ASSERT_EQ(midi_events[0].message[2], 64);   // Value
}

TEST(generate_all_notes_off) {
    std::string script = createTempLuaScript(R"(
        function init(context)
        end

        function process_event(track, event)
            if event.switch then
                stopall(0)
            end
        end
    )");

    LuaContext ctx;
    ctx.setChannel(0);
    ASSERT_TRUE(ctx.loadScript(script));

    LuaInitContext init_ctx = {120, 0, 0};
    ASSERT_TRUE(ctx.callInit(init_ctx));

    Event evt;
    evt.setSwitch(true);

    auto midi_events = ctx.callProcessEvent(0, evt);

    ASSERT_EQ(midi_events.size(), 1);
    ASSERT_EQ(midi_events[0].message[0], 0xB0); // CC, channel 0
    ASSERT_EQ(midi_events[0].message[1], 123);  // All Notes Off
    ASSERT_EQ(midi_events[0].message[2], 0);
}

// ============================================================================
// Lua 5.1 Compatibility Tests (Features NOT to use)
// ============================================================================

TEST(lua_5_1_no_integer_division) {
    // Lua 5.1 doesn't have // operator (added in 5.3)
    // This test verifies our scripts don't rely on it
    std::string script = createTempLuaScript(R"(
        function init(context)
        end

        function process_event(track, event)
            -- Use / instead of //
            local half = event.pots[1] / 2
            note(60, math.floor(half))
        end
    )");

    LuaContext ctx;
    ctx.setChannel(0);
    ASSERT_TRUE(ctx.loadScript(script));  // Should work in both 5.1 and 5.4
}

TEST(lua_5_1_no_bitwise_operators) {
    // Lua 5.1 doesn't have & | ~ operators (added in 5.3)
    // This test verifies we use math.floor() instead
    std::string script = createTempLuaScript(R"(
        function init(context)
        end

        function process_event(track, event)
            -- Use math operations instead of bitwise
            local value = math.floor(event.pots[1] / 2) * 2
            note(60, value)
        end
    )");

    LuaContext ctx;
    ctx.setChannel(0);
    ASSERT_TRUE(ctx.loadScript(script));  // Should work in both 5.1 and 5.4
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "=== Lua Integration Tests ===" << std::endl;
    std::cout << std::endl;

    // Version compatibility
    run_test_lua_version_detected();
    run_test_lua_ok_constant_defined();

    // Context tests
    run_test_lua_context_creation();
    run_test_lua_api_registration();

    // Script loading
    run_test_load_simple_script();
    run_test_reject_script_without_init();
    run_test_reject_script_without_process_event();
    run_test_reject_script_with_syntax_error();

    // Init
    run_test_call_init_with_context();

    // Event processing
    run_test_process_event_with_switch_on();
    run_test_process_event_with_switch_off();
    run_test_process_event_reads_pot_values();

    // MIDI generation
    run_test_generate_control_change();
    run_test_generate_all_notes_off();

    // Lua 5.1 compatibility
    run_test_lua_5_1_no_integer_division();
    run_test_lua_5_1_no_bitwise_operators();

    std::cout << std::endl;
    std::cout << "=== Test Results ===" << std::endl;
    std::cout << "Total:  " << test_count << std::endl;
    std::cout << "Passed: " << pass_count << std::endl;
    std::cout << "Failed: " << fail_count << std::endl;

    return (fail_count == 0) ? 0 : 1;
}
