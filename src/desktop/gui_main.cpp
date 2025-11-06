#include "../core/song.h"
#include "../core/engine.h"
#include "../lua_bridge/mode_loader.h"
#include "desktop_hardware.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include <SDL.h>

#include <iostream>
#include <memory>
#include <algorithm>
#include <cmath>

using namespace gruvbok;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Helper function to draw a circular knob widget
bool Knob(const char* label, int* value, int min_val, int max_val, const char* display_value = nullptr, float radius = 30.0f) {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    float line_height = ImGui::GetTextLineHeight();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 center = ImVec2(pos.x + radius, pos.y + radius);

    ImGui::InvisibleButton(label, ImVec2(radius * 2.0f, radius * 2.0f + line_height * 2 + style.ItemInnerSpacing.y));
    bool is_active = ImGui::IsItemActive();
    bool is_hovered = ImGui::IsItemHovered();
    bool value_changed = false;

    if (is_active && io.MouseDelta.y != 0.0f) {
        float delta = -io.MouseDelta.y * (max_val - min_val) / 200.0f;
        *value = std::clamp(*value + (int)delta, min_val, max_val);
        value_changed = true;
    }

    // Calculate angle for current value (-135 to +135 degrees)
    float t = (float)(*value - min_val) / (float)(max_val - min_val);
    float angle = (t * 270.0f - 135.0f) * (M_PI / 180.0f);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // Draw outer circle
    draw_list->AddCircleFilled(center, radius, ImGui::GetColorU32(ImGuiCol_FrameBg), 32);
    draw_list->AddCircle(center, radius, ImGui::GetColorU32(is_active ? ImGuiCol_FrameBgActive : is_hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_Border), 32, 2.0f);

    // Draw value indicator line
    float indicator_x = center.x + cosf(angle) * (radius * 0.7f);
    float indicator_y = center.y + sinf(angle) * (radius * 0.7f);
    draw_list->AddLine(center, ImVec2(indicator_x, indicator_y), ImGui::GetColorU32(ImGuiCol_SliderGrabActive), 3.0f);

    // Draw center dot
    draw_list->AddCircleFilled(center, radius * 0.15f, ImGui::GetColorU32(ImGuiCol_Button), 12);

    // Draw label
    ImVec2 label_size = ImGui::CalcTextSize(label);
    draw_list->AddText(ImVec2(center.x - label_size.x * 0.5f, pos.y + radius * 2.0f + style.ItemInnerSpacing.y),
                      ImGui::GetColorU32(ImGuiCol_Text), label);

    // Draw converted value below label
    const char* value_text = display_value ? display_value : "";
    ImVec2 value_size = ImGui::CalcTextSize(value_text);
    draw_list->AddText(ImVec2(center.x - value_size.x * 0.5f, pos.y + radius * 2.0f + line_height + style.ItemInnerSpacing.y),
                      ImGui::GetColorU32(ImGuiCol_TextDisabled), value_text);

    return value_changed;
}

// Get slider labels for current mode
const char* GetSliderLabel(int slider_index, int mode_number) {
    if (mode_number == 0) {  // Song/Pattern Sequencer
        const char* labels[] = {"Pattern", "---", "---", "---"};
        return labels[slider_index];
    } else if (mode_number == 1) {  // Drums
        const char* labels[] = {"Velocity", "Length", "S3", "S4"};
        return labels[slider_index];
    } else if (mode_number == 2) {  // Acid
        const char* labels[] = {"Pitch", "Length", "Slide", "Filter"};
        return labels[slider_index];
    } else if (mode_number == 3) {  // Chords
        const char* labels[] = {"Root", "Type", "Velocity", "Length"};
        return labels[slider_index];
    } else if (mode_number == 4) {  // Arpeggiator
        const char* labels[] = {"Root", "Pattern", "Velocity", "Length"};
        return labels[slider_index];
    } else if (mode_number == 5) {  // Euclidean
        const char* labels[] = {"Hits", "Rotate", "Pitch", "Velocity"};
        return labels[slider_index];
    } else if (mode_number == 6) {  // Random
        const char* labels[] = {"Prob", "Center", "Range", "Velocity"};
        return labels[slider_index];
    } else if (mode_number == 7) {  // Sample & Hold
        const char* labels[] = {"Rate", "Quant", "Glitch", "Mod"};
        return labels[slider_index];
    } else {
        const char* labels[] = {"S1", "S2", "S3", "S4"};
        return labels[slider_index];
    }
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create window
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow(
        "GRUVBOK Hardware Simulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280,
        720,
        window_flags
    );

    if (!window) {
        std::cerr << "Error creating window: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Error creating renderer: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // Initialize GRUVBOK
    auto hardware = std::make_unique<DesktopHardware>();
    if (!hardware->init()) {
        std::cerr << "Failed to initialize hardware" << std::endl;
        return 1;
    }

    // Set default rotary pot values
    // Tempo formula: tempo = 60 + (r2 * 180) / 127
    // For 120 BPM: r2 = (120-60)*127/180 = 42.33 ≈ 42
    hardware->simulateRotaryPot(0, 0);    // R1: Mode 0
    hardware->simulateRotaryPot(1, 42);   // R2: 120 BPM (calculated: 60 + 42*180/127 = 119 BPM)
    hardware->simulateRotaryPot(2, 0);    // R3: Pattern 0
    hardware->simulateRotaryPot(3, 0);    // R4: Track 0

    auto song = std::make_unique<Song>();
    auto mode_loader = std::make_unique<ModeLoader>();

    int loaded = mode_loader->loadModesFromDirectory("modes", 120);
    if (loaded > 0) {
        hardware->addLog("Loaded " + std::to_string(loaded) + " mode(s) from modes/ directory");
    } else {
        hardware->addLog("Warning: No modes loaded");
    }

    auto engine = std::make_unique<Engine>(song.get(), hardware.get(), mode_loader.get());

    // Mode 0: Song/Pattern Sequencer - Default pattern chain
    // Patterns 0-3 repeating 4 times each (steps 0-3, 4-7, 8-11, 12-15)
    Mode& mode0 = song->getMode(0);
    Pattern& song_pattern = mode0.getPattern(0);  // Mode 0 always uses pattern 0

    // Steps 0-3: Pattern 0
    for (int step = 0; step < 4; step++) {
        Event& event = song_pattern.getEvent(0, step);
        event.setSwitch(true);
        event.setPot(0, 0);  // S1 = 0 -> Pattern 0
    }

    // Steps 4-7: Pattern 1
    for (int step = 4; step < 8; step++) {
        Event& event = song_pattern.getEvent(0, step);
        event.setSwitch(true);
        event.setPot(0, 4);  // S1 = 4 -> Pattern 1
    }

    // Steps 8-11: Pattern 2
    for (int step = 8; step < 12; step++) {
        Event& event = song_pattern.getEvent(0, step);
        event.setSwitch(true);
        event.setPot(0, 8);  // S1 = 8 -> Pattern 2
    }

    // Steps 12-15: Pattern 3
    for (int step = 12; step < 16; step++) {
        Event& event = song_pattern.getEvent(0, step);
        event.setSwitch(true);
        event.setPot(0, 12);  // S1 = 12 -> Pattern 3
    }

    Mode& mode1 = song->getMode(1);
    Pattern& pattern0 = mode1.getPattern(0);

    // Kick pattern
    for (int step : {0, 4, 8, 12}) {
        Event& event = pattern0.getEvent(0, step);
        event.setSwitch(true);
        event.setPot(0, 100);
        event.setPot(1, 50);
    }

    // Snare pattern
    for (int step : {4, 12}) {
        Event& event = pattern0.getEvent(1, step);
        event.setSwitch(true);
        event.setPot(0, 90);
        event.setPot(1, 30);
    }

    // Hi-hat pattern
    for (int step = 0; step < 16; step += 2) {
        Event& event = pattern0.getEvent(2, step);
        event.setSwitch(true);
        event.setPot(0, 70);
        event.setPot(1, 20);
    }

    // Acid bassline on mode 2 - Track 0 only, S1 controls pitch
    Mode& mode2 = song->getMode(2);
    Pattern& acid_pattern = mode2.getPattern(0);

    // Classic acid pattern - all on track 0, S1 varies to create melody
    // S1 mapping: 0-41 = octave 1, 42-83 = octave 2, 84-127 = octave 3
    // Within each range: C, Eb, F, G, Bb (pentatonic)

    // Step 0: Root (C2) - octave 2
    acid_pattern.getEvent(0, 0).setSwitch(true);
    acid_pattern.getEvent(0, 0).setPot(0, 42);   // S1: C in octave 2
    acid_pattern.getEvent(0, 0).setPot(1, 40);   // S2: Short note
    acid_pattern.getEvent(0, 0).setPot(2, 10);   // S3: Minimal slide
    acid_pattern.getEvent(0, 0).setPot(3, 60);   // S4: Filter

    // Step 3: Fifth (G2)
    acid_pattern.getEvent(0, 3).setSwitch(true);
    acid_pattern.getEvent(0, 3).setPot(0, 67);   // S1: G in octave 2
    acid_pattern.getEvent(0, 3).setPot(1, 35);   // S2: Short note
    acid_pattern.getEvent(0, 3).setPot(2, 60);   // S3: Some slide
    acid_pattern.getEvent(0, 3).setPot(3, 80);   // S4: Higher filter

    // Step 4: Octave up (C3)
    acid_pattern.getEvent(0, 4).setSwitch(true);
    acid_pattern.getEvent(0, 4).setPot(0, 84);   // S1: C in octave 3
    acid_pattern.getEvent(0, 4).setPot(1, 30);   // S2: Short
    acid_pattern.getEvent(0, 4).setPot(2, 100);  // S3: Big slide from prev note
    acid_pattern.getEvent(0, 4).setPot(3, 110);  // S4: High filter (accent)

    // Step 6: Fourth (F2)
    acid_pattern.getEvent(0, 6).setSwitch(true);
    acid_pattern.getEvent(0, 6).setPot(0, 59);   // S1: F in octave 2
    acid_pattern.getEvent(0, 6).setPot(1, 40);   // S2: Medium note
    acid_pattern.getEvent(0, 6).setPot(2, 20);   // S3: Slight slide
    acid_pattern.getEvent(0, 6).setPot(3, 70);   // S4: Mid filter

    // Step 8: Back to root (C2)
    acid_pattern.getEvent(0, 8).setSwitch(true);
    acid_pattern.getEvent(0, 8).setPot(0, 42);   // S1: C in octave 2
    acid_pattern.getEvent(0, 8).setPot(1, 50);   // S2: Longer note
    acid_pattern.getEvent(0, 8).setPot(2, 5);    // S3: No slide
    acid_pattern.getEvent(0, 8).setPot(3, 50);   // S4: Lower filter

    // Step 10: Minor third (Eb2)
    acid_pattern.getEvent(0, 10).setSwitch(true);
    acid_pattern.getEvent(0, 10).setPot(0, 50);  // S1: Eb in octave 2
    acid_pattern.getEvent(0, 10).setPot(1, 35);  // S2: Short
    acid_pattern.getEvent(0, 10).setPot(2, 40);  // S3: Medium slide
    acid_pattern.getEvent(0, 10).setPot(3, 85);  // S4: High filter

    // Step 12: Fifth again (G2)
    acid_pattern.getEvent(0, 12).setSwitch(true);
    acid_pattern.getEvent(0, 12).setPot(0, 67);  // S1: G in octave 2
    acid_pattern.getEvent(0, 12).setPot(1, 40);  // S2: Medium
    acid_pattern.getEvent(0, 12).setPot(2, 30);  // S3: Some slide
    acid_pattern.getEvent(0, 12).setPot(3, 75);  // S4: Mid-high filter

    // Step 14: Lower octave root (C1) for depth
    acid_pattern.getEvent(0, 14).setSwitch(true);
    acid_pattern.getEvent(0, 14).setPot(0, 8);   // S1: C in octave 1 (lower)
    acid_pattern.getEvent(0, 14).setPot(1, 60);  // S2: Long note
    acid_pattern.getEvent(0, 14).setPot(2, 0);   // S3: No slide
    acid_pattern.getEvent(0, 14).setPot(3, 40);  // S4: Low filter (bass)

    // Chord progression on mode 3 - Track 0
    Mode& mode3 = song->getMode(3);
    Pattern& chord_pattern = mode3.getPattern(0);

    // Simple I-IV-V-I progression in C major
    // S2 chord types: 0-7 = Major, 8-15 = Minor, etc.

    // Step 0: C Major (root = C4 = 60, type = major)
    chord_pattern.getEvent(0, 0).setSwitch(true);
    chord_pattern.getEvent(0, 0).setPot(0, 60);   // S1: Root = C4
    chord_pattern.getEvent(0, 0).setPot(1, 0);    // S2: Major chord
    chord_pattern.getEvent(0, 0).setPot(2, 90);   // S3: Velocity
    chord_pattern.getEvent(0, 0).setPot(3, 80);   // S4: Length

    // Step 4: F Major (root = F4 = 65)
    chord_pattern.getEvent(0, 4).setSwitch(true);
    chord_pattern.getEvent(0, 4).setPot(0, 65);   // S1: Root = F4
    chord_pattern.getEvent(0, 4).setPot(1, 0);    // S2: Major chord
    chord_pattern.getEvent(0, 4).setPot(2, 85);   // S3: Velocity
    chord_pattern.getEvent(0, 4).setPot(3, 80);   // S4: Length

    // Step 8: G Major (root = G4 = 67)
    chord_pattern.getEvent(0, 8).setSwitch(true);
    chord_pattern.getEvent(0, 8).setPot(0, 67);   // S1: Root = G4
    chord_pattern.getEvent(0, 8).setPot(1, 0);    // S2: Major chord
    chord_pattern.getEvent(0, 8).setPot(2, 95);   // S3: Velocity
    chord_pattern.getEvent(0, 8).setPot(3, 80);   // S4: Length

    // Step 12: C Major (back to root)
    chord_pattern.getEvent(0, 12).setSwitch(true);
    chord_pattern.getEvent(0, 12).setPot(0, 60);  // S1: Root = C4
    chord_pattern.getEvent(0, 12).setPot(1, 0);   // S2: Major chord
    chord_pattern.getEvent(0, 12).setPot(2, 100); // S3: Velocity
    chord_pattern.getEvent(0, 12).setPot(3, 100); // S4: Longer length

    hardware->addLog("Demo pattern created (drums ch1, acid ch2, chords ch3)");
    engine->start();
    hardware->addLog("Engine started - playback running");

    // Main loop
    bool running = true;
    while (running) {
        // Poll events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                running = false;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
                running = false;
        }

        // Update engine
        engine->update();

        // Start ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Main GUI window
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_FirstUseEver);
            ImGui::Begin("GRUVBOK Hardware Simulator", nullptr, ImGuiWindowFlags_NoCollapse);

            // MIDI Output Port Selector
            ImGui::Text("MIDI Output:");
            ImGui::SameLine();

            int port_count = hardware->getMidiPortCount();
            int current_port = hardware->getCurrentMidiPort();

            std::string preview = current_port < 0 ? "Virtual Port" : hardware->getMidiPortName(current_port);
            if (ImGui::BeginCombo("##MIDIPort", preview.c_str())) {
                // Virtual port option
                bool is_selected = (current_port < 0);
                if (ImGui::Selectable("Virtual Port", is_selected)) {
                    hardware->selectMidiPort(-1);
                }

                // Real ports
                for (int i = 0; i < port_count; i++) {
                    is_selected = (current_port == i);
                    std::string port_name = hardware->getMidiPortName(i);
                    if (ImGui::Selectable(port_name.c_str(), is_selected)) {
                        hardware->selectMidiPort(i);
                    }
                }
                ImGui::EndCombo();
            }

            // Mirror Mode (MIDI Input)
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();

            bool mirror_mode = hardware->isMirrorModeEnabled();
            if (ImGui::Checkbox("Mirror Mode", &mirror_mode)) {
                hardware->setMirrorMode(mirror_mode);
            }

            // MIDI Input Port Selector (only show if mirror mode enabled)
            if (mirror_mode) {
                ImGui::Text("MIDI Input:");
                ImGui::SameLine();

                int input_port_count = hardware->getMidiInputPortCount();
                int current_input_port = hardware->getCurrentMidiInputPort();

                std::string input_preview = current_input_port < 0 ? "Select Input..." : hardware->getMidiInputPortName(current_input_port);
                if (ImGui::BeginCombo("##MIDIInputPort", input_preview.c_str())) {
                    for (int i = 0; i < input_port_count; i++) {
                        bool is_selected = (current_input_port == i);
                        std::string port_name = hardware->getMidiInputPortName(i);
                        if (ImGui::Selectable(port_name.c_str(), is_selected)) {
                            hardware->selectMidiInputPort(i);
                        }
                    }
                    ImGui::EndCombo();
                }
            }

            ImGui::Separator();

            // LED tempo indicator
            bool led_state = hardware->getLED();
            ImVec4 led_color = led_state ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
            ImGui::Text("Tempo LED:");
            ImGui::SameLine();
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 led_pos = ImGui::GetCursorScreenPos();
            float led_radius = 8.0f;
            draw_list->AddCircleFilled(ImVec2(led_pos.x + led_radius, led_pos.y + led_radius),
                                      led_radius,
                                      ImGui::GetColorU32(led_color), 16);
            ImGui::Dummy(ImVec2(led_radius * 2, led_radius * 2));

            ImGui::Separator();

            // Global controls (R1-R4) as knobs
            ImGui::Text("Global Controls");
            ImGui::BeginGroup();

            int r1 = hardware->readRotaryPot(0);
            int r2 = hardware->readRotaryPot(1);
            int r3 = hardware->readRotaryPot(2);
            int r4 = hardware->readRotaryPot(3);

            // Calculate converted values for display
            int mode_val = (r1 * 15) / 128;
            int tempo_val = 60 + (r2 * 180) / 127;
            int pattern_val = (r3 * 32) / 128;
            int track_val = (r4 * 8) / 128;

            char mode_str[16], tempo_str[16], pattern_str[16], track_str[16];
            snprintf(mode_str, sizeof(mode_str), "%d", mode_val);
            snprintf(tempo_str, sizeof(tempo_str), "%d BPM", tempo_val);
            snprintf(pattern_str, sizeof(pattern_str), "%d", pattern_val);
            snprintf(track_str, sizeof(track_str), "%d", track_val);

            float knob_width = 100.0f;

            if (Knob("Mode", &r1, 0, 127, mode_str, 35.0f)) hardware->simulateRotaryPot(0, r1);
            ImGui::SameLine(0, 0);
            ImGui::Dummy(ImVec2(knob_width - 70, 0));
            ImGui::SameLine(0, 0);
            if (Knob("Tempo", &r2, 0, 127, tempo_str, 35.0f)) hardware->simulateRotaryPot(1, r2);
            ImGui::SameLine(0, 0);
            ImGui::Dummy(ImVec2(knob_width - 70, 0));
            ImGui::SameLine(0, 0);
            if (Knob("Pattern", &r3, 0, 127, pattern_str, 35.0f)) hardware->simulateRotaryPot(2, r3);
            ImGui::SameLine(0, 0);
            ImGui::Dummy(ImVec2(knob_width - 70, 0));
            ImGui::SameLine(0, 0);
            if (Knob("Track", &r4, 0, 127, track_str, 35.0f)) hardware->simulateRotaryPot(3, r4);

            ImGui::EndGroup();
            ImGui::Separator();

            // Slider pots (S1-S4) with mode-specific labels
            int current_mode = engine->getCurrentMode();
            LuaContext* lua_mode = mode_loader->getMode(current_mode);
            std::string mode_name = lua_mode && lua_mode->isValid() ? lua_mode->getModeName() : "Unknown";
            ImGui::Text("Mode %d: %s", current_mode, mode_name.c_str());
            ImGui::BeginGroup();

            int s1 = hardware->readSliderPot(0);
            int s2 = hardware->readSliderPot(1);
            int s3 = hardware->readSliderPot(2);
            int s4 = hardware->readSliderPot(3);

            char s1_label[64], s2_label[64], s3_label[64], s4_label[64];
            snprintf(s1_label, sizeof(s1_label), "%s\n%d", GetSliderLabel(0, current_mode), s1);
            snprintf(s2_label, sizeof(s2_label), "%s\n%d", GetSliderLabel(1, current_mode), s2);
            snprintf(s3_label, sizeof(s3_label), "%s\n%d", GetSliderLabel(2, current_mode), s3);
            snprintf(s4_label, sizeof(s4_label), "%s\n%d", GetSliderLabel(3, current_mode), s4);

            float slider_width = 100.0f;

            // Sliders now only set values when you press a button (parameter lock)
            // Just update the hardware values for display/readback
            if (ImGui::VSliderInt("##S1", ImVec2(50, 180), &s1, 0, 127, s1_label)) {
                hardware->simulateSliderPot(0, s1);
            }
            ImGui::SameLine(0, 0);
            ImGui::Dummy(ImVec2(slider_width - 50, 0));
            ImGui::SameLine(0, 0);
            if (ImGui::VSliderInt("##S2", ImVec2(50, 180), &s2, 0, 127, s2_label)) {
                hardware->simulateSliderPot(1, s2);
            }
            ImGui::SameLine(0, 0);
            ImGui::Dummy(ImVec2(slider_width - 50, 0));
            ImGui::SameLine(0, 0);
            if (ImGui::VSliderInt("##S3", ImVec2(50, 180), &s3, 0, 127, s3_label)) {
                hardware->simulateSliderPot(2, s3);
            }
            ImGui::SameLine(0, 0);
            ImGui::Dummy(ImVec2(slider_width - 50, 0));
            ImGui::SameLine(0, 0);
            if (ImGui::VSliderInt("##S4", ImVec2(50, 180), &s4, 0, 127, s4_label)) {
                hardware->simulateSliderPot(3, s4);
            }

            ImGui::EndGroup();
            ImGui::Separator();

            // Pattern grid visualization
            ImGui::Text("Pattern Grid (Track %d)", engine->getCurrentTrack());
            ImGui::BeginGroup();

            Mode& editing_mode = song->getMode(engine->getCurrentMode());
            Pattern& current_pattern = editing_mode.getPattern(engine->getCurrentPattern());
            Track& current_track = current_pattern.getTrack(engine->getCurrentTrack());

            float step_width = 50.0f;

            // Track which button is being held for live editing
            static int held_button = -1;

            for (int step = 0; step < 16; step++) {
                Event& evt = current_track.getEvent(step);
                bool has_event = evt.getSwitch();

                // Color: yellow if held, red if playing, green if active, gray if empty
                ImVec4 color;
                if (held_button == step) {
                    color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow for held button
                } else if (step == engine->getCurrentStep()) {
                    color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red for current step
                } else if (has_event) {
                    color = ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green for active
                } else {
                    color = ImVec4(0.3f, 0.3f, 0.3f, 1.0f); // Gray for empty
                }

                ImGui::PushStyleColor(ImGuiCol_Button, color);
                char btn_label[8];
                snprintf(btn_label, sizeof(btn_label), "%d", step + 1);

                ImGui::Button(btn_label, ImVec2(40, 40));
                bool is_held = ImGui::IsItemActive();

                // Button clicked (just pressed)
                if (ImGui::IsItemActivated()) {
                    held_button = step;

                    // Toggle event on first press
                    bool new_state = !evt.getSwitch();
                    evt.setSwitch(new_state);

                    // If turning ON, lock current slider values
                    if (new_state) {
                        evt.setPot(0, hardware->readSliderPot(0));
                        evt.setPot(1, hardware->readSliderPot(1));
                        evt.setPot(2, hardware->readSliderPot(2));
                        evt.setPot(3, hardware->readSliderPot(3));
                    }

                    char log_msg[128];
                    snprintf(log_msg, sizeof(log_msg),
                             "Step %d: %s (S1=%d S2=%d S3=%d S4=%d)",
                             step + 1, new_state ? "ON" : "OFF",
                             evt.getPot(0), evt.getPot(1), evt.getPot(2), evt.getPot(3));
                    hardware->addLog(log_msg);
                }

                // Button being held - continuously update pot values
                if (is_held && held_button == step && evt.getSwitch()) {
                    evt.setPot(0, hardware->readSliderPot(0));
                    evt.setPot(1, hardware->readSliderPot(1));
                    evt.setPot(2, hardware->readSliderPot(2));
                    evt.setPot(3, hardware->readSliderPot(3));
                }

                // Button released
                if (!is_held && held_button == step) {
                    held_button = -1;

                    // Log final values on release
                    if (evt.getSwitch()) {
                        char log_msg[128];
                        snprintf(log_msg, sizeof(log_msg),
                                 "Step %d locked: S1=%d S2=%d S3=%d S4=%d",
                                 step + 1,
                                 evt.getPot(0), evt.getPot(1), evt.getPot(2), evt.getPot(3));
                        hardware->addLog(log_msg);
                    }
                }

                ImGui::PopStyleColor();

                if (step < 15) {
                    ImGui::SameLine(0, 0);
                    ImGui::Dummy(ImVec2(step_width - 40, 0));
                    ImGui::SameLine(0, 0);
                }
                if (step == 7) ImGui::NewLine();
            }

            ImGui::EndGroup();
            ImGui::End();
        }

        // Song Data Explorer window
        {
            ImGui::SetNextWindowPos(ImVec2(850, 0), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(430, 520), ImGuiCond_FirstUseEver);
            ImGui::Begin("Song Data Explorer", nullptr, ImGuiWindowFlags_NoCollapse);

            ImGui::Text("Navigate the entire song structure");
            ImGui::Separator();

            // Mode/Pattern/Track selector for focused view
            static int explorer_mode = 0;
            static int explorer_pattern = 0;
            static int explorer_track = 0;

            ImGui::PushItemWidth(100);
            ImGui::DragInt("Mode", &explorer_mode, 0.1f, 0, 14);
            ImGui::SameLine();
            ImGui::DragInt("Pattern", &explorer_pattern, 0.1f, 0, 31);
            ImGui::SameLine();
            ImGui::DragInt("Track", &explorer_track, 0.1f, 0, 7);
            ImGui::PopItemWidth();

            ImGui::Separator();

            // Show events for selected mode/pattern/track in a table
            Mode& exp_mode = song->getMode(explorer_mode);
            Pattern& exp_pattern = exp_mode.getPattern(explorer_pattern);
            Track& exp_track = exp_pattern.getTrack(explorer_track);

            ImGui::Text("Events: Mode %d, Pattern %d, Track %d", explorer_mode, explorer_pattern, explorer_track);

            // Table with event data
            if (ImGui::BeginTable("EventTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupColumn("Step", ImGuiTableColumnFlags_WidthFixed, 40.0f);
                ImGui::TableSetupColumn("ON", ImGuiTableColumnFlags_WidthFixed, 35.0f);
                ImGui::TableSetupColumn("S1", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("S2", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("S3", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("S4", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableHeadersRow();

                for (int step = 0; step < 16; step++) {
                    const Event& evt = exp_track.getEvent(step);
                    ImGui::TableNextRow();

                    // Highlight current step if viewing current mode/pattern/track
                    bool is_current = (explorer_mode == engine->getCurrentMode() &&
                                      explorer_pattern == engine->getCurrentPattern() &&
                                      explorer_track == engine->getCurrentTrack() &&
                                      step == engine->getCurrentStep());
                    if (is_current) {
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.6f, 0.3f)));
                    }

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", step + 1);

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", evt.getSwitch() ? "•" : "");

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", evt.getPot(0));

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", evt.getPot(1));

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", evt.getPot(2));

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", evt.getPot(3));
                }

                ImGui::EndTable();
            }

            ImGui::Separator();

            // Quick stats
            ImGui::Text("Song Overview:");
            ImGui::BulletText("15 Modes (0-14)");
            ImGui::BulletText("32 Patterns per Mode");
            ImGui::BulletText("8 Tracks per Pattern");
            ImGui::BulletText("16 Events per Track");

            int total_events = 15 * 32 * 8 * 16;
            ImGui::Text("Total capacity: %d events", total_events);

            ImGui::End();
        }

        // Log window
        {
            ImGui::SetNextWindowPos(ImVec2(0, 530), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(1280, 190), ImGuiCond_FirstUseEver);
            ImGui::Begin("System Log", nullptr, ImGuiWindowFlags_NoCollapse);

            if (ImGui::Button("Clear Log")) {
                hardware->clearLog();
            }

            ImGui::Separator();

            // Scrollable log area
            ImGui::BeginChild("LogScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

            const auto& log_messages = hardware->getLogMessages();
            for (const auto& message : log_messages) {
                ImGui::TextUnformatted(message.c_str());
            }

            // Auto-scroll to bottom
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);

            ImGui::EndChild();
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);

        // Small delay to avoid spinning
        SDL_Delay(10);
    }

    // Cleanup
    engine->stop();
    hardware->shutdown();

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
