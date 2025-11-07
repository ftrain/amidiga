# Console executable (original)
add_executable(gruvbok-console
    main.cpp
    desktop_hardware.cpp
)

target_include_directories(gruvbok-console PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${LUA_INCLUDE_DIR}
)

target_link_libraries(gruvbok-console PRIVATE
    gruvbok_lua  # Includes gruvbok_hardware and gruvbok_core transitively
    rtmidi
    ${LUA_LIBRARIES}
)

# GUI executable (new!)
find_package(SDL2 REQUIRED)

# macOS app bundle configuration
if(APPLE)
    set(MACOSX_BUNDLE_BUNDLE_NAME "GRUVBOK")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.gruvbok.sequencer")
    set(MACOSX_BUNDLE_ICON_FILE "gruvbok.icns")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2025 GRUVBOK")

    # Set icon file path
    set(ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/resources/gruvbok.icns)

    add_executable(gruvbok MACOSX_BUNDLE
        gui_main.cpp
        desktop_hardware.cpp
        ${ICON_FILE}
    )

    # Configure bundle properties
    set_target_properties(gruvbok PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    )

    # Add icon to Resources folder in bundle
    set_source_files_properties(${ICON_FILE} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )

    # Copy modes folder into bundle
    add_custom_command(TARGET gruvbok POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/modes
        $<TARGET_BUNDLE_CONTENT_DIR:gruvbok>/Resources/modes
        COMMENT "Copying modes into app bundle"
    )
else()
    add_executable(gruvbok
        gui_main.cpp
        desktop_hardware.cpp
    )
endif()

target_include_directories(gruvbok PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${LUA_INCLUDE_DIR}
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(gruvbok PRIVATE
    gruvbok_lua  # Includes gruvbok_hardware and gruvbok_core transitively
    rtmidi
    imgui
    ${LUA_LIBRARIES}
    ${SDL2_LIBRARIES}
)

# Set output directory
set_target_properties(gruvbok gruvbok-console PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Installation
install(TARGETS gruvbok gruvbok-console DESTINATION bin)
