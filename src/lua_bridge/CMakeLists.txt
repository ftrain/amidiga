# Lua bridge library
add_library(gruvbok_lua STATIC
    lua_api.cpp
    lua_context.cpp
    mode_loader.cpp
)

target_include_directories(gruvbok_lua PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(gruvbok_lua PUBLIC
    gruvbok_core
    gruvbok_hardware
    ${LUA_LIBRARIES}
)
