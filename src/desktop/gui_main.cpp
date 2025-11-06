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
bool Knob(const char* label, int* value, int min_val, int max_val, float radius = 30.0f) {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    float line_height = ImGui::GetTextLineHeight();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 center = ImVec2(pos.x + radius, pos.y + radius);
    float gamma = ImGui::GetColorU32(ImGuiCol_ButtonActive);

    ImGui::InvisibleButton(label, ImVec2(radius * 2.0f, radius * 2.0f + line_height + style.ItemInnerSpacing.y));
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

    // Draw label and value
    char display_text[64];
    snprintf(display_text, sizeof(display_text), "%s: %d", label, *value);
    ImVec2 text_size = ImGui::CalcTextSize(display_text);
    draw_list->AddText(ImVec2(center.x - text_size.x * 0.5f, pos.y + radius * 2.0f + style.ItemInnerSpacing.y),
                      ImGui::GetColorU32(ImGuiCol_Text), display_text);

    return value_changed;
}

// Get slider labels for current mode
const char* GetSliderLabel(int slider_index, int mode_number) {
    if (mode_number == 1) {  // Drums
        const char* labels[] = {"Velocity", "Length", "S3", "S4"};
        return labels[slider_index];
    } else if (mode_number == 2) {  // Acid
        const char* labels[] = {"Pitch", "Length", "Slide", "Filter"};
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
    hardware->simulateRotaryPot(0, 0);    // R1: Mode 0
    hardware->simulateRotaryPot(1, 64);   // R2: 120 BPM (middle of range)
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

    hardware->addLog("Demo pattern created (drums on ch1, acid bass on ch2)");
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

            // Status display
            ImGui::Text("Status: %s", engine->isPlaying() ? "PLAYING" : "STOPPED");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "(All 15 modes playing on MIDI channels 0-14)");

            ImGui::Text("Tempo: %d BPM", engine->getTempo());
            ImGui::SameLine();
            ImGui::Text("| Editing Mode: %d", engine->getCurrentMode());
            ImGui::SameLine();
            ImGui::Text("| Pattern: %d", engine->getCurrentPattern());
            ImGui::SameLine();
            ImGui::Text("| Track: %d", engine->getCurrentTrack());
            ImGui::SameLine();
            ImGui::Text("| Step: %d", engine->getCurrentStep());

            ImGui::Separator();

            // MIDI Port Selector
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

            ImGui::Separator();

            // Global controls (R1-R4) as knobs
            ImGui::Text("Global Controls");

            int r1 = hardware->readRotaryPot(0);
            int r2 = hardware->readRotaryPot(1);
            int r3 = hardware->readRotaryPot(2);
            int r4 = hardware->readRotaryPot(3);

            if (Knob("Mode", &r1, 0, 127, 35.0f)) hardware->simulateRotaryPot(0, r1);
            ImGui::SameLine(0, 20);
            if (Knob("Tempo", &r2, 0, 127, 35.0f)) hardware->simulateRotaryPot(1, r2);
            ImGui::SameLine(0, 20);
            if (Knob("Pattern", &r3, 0, 127, 35.0f)) hardware->simulateRotaryPot(2, r3);
            ImGui::SameLine(0, 20);
            if (Knob("Track", &r4, 0, 127, 35.0f)) hardware->simulateRotaryPot(3, r4);

            ImGui::Separator();

            // Slider pots (S1-S4) with mode-specific labels
            int current_mode = engine->getCurrentMode();
            ImGui::Text("Slider Pots (Mode %d)", current_mode);

            int s1 = hardware->readSliderPot(0);
            int s2 = hardware->readSliderPot(1);
            int s3 = hardware->readSliderPot(2);
            int s4 = hardware->readSliderPot(3);

            char s1_label[64], s2_label[64], s3_label[64], s4_label[64];
            snprintf(s1_label, sizeof(s1_label), "%s\n%d", GetSliderLabel(0, current_mode), s1);
            snprintf(s2_label, sizeof(s2_label), "%s\n%d", GetSliderLabel(1, current_mode), s2);
            snprintf(s3_label, sizeof(s3_label), "%s\n%d", GetSliderLabel(2, current_mode), s3);
            snprintf(s4_label, sizeof(s4_label), "%s\n%d", GetSliderLabel(3, current_mode), s4);

            // Track previous slider values for change detection
            static int prev_s1 = -1, prev_s2 = -1, prev_s3 = -1, prev_s4 = -1;

            if (ImGui::VSliderInt("##S1", ImVec2(50, 180), &s1, 0, 127, s1_label)) {
                hardware->simulateSliderPot(0, s1);
                if (prev_s1 != s1) {
                    char log_msg[128];
                    snprintf(log_msg, sizeof(log_msg), "Mode %d, Track %d, Step %d: %s changed to %d",
                             engine->getCurrentMode(), engine->getCurrentTrack(), engine->getCurrentStep() + 1,
                             GetSliderLabel(0, current_mode), s1);
                    hardware->addLog(log_msg);
                    prev_s1 = s1;
                }
            }
            ImGui::SameLine();
            if (ImGui::VSliderInt("##S2", ImVec2(50, 180), &s2, 0, 127, s2_label)) {
                hardware->simulateSliderPot(1, s2);
                if (prev_s2 != s2) {
                    char log_msg[128];
                    snprintf(log_msg, sizeof(log_msg), "Mode %d, Track %d, Step %d: %s changed to %d",
                             engine->getCurrentMode(), engine->getCurrentTrack(), engine->getCurrentStep() + 1,
                             GetSliderLabel(1, current_mode), s2);
                    hardware->addLog(log_msg);
                    prev_s2 = s2;
                }
            }
            ImGui::SameLine();
            if (ImGui::VSliderInt("##S3", ImVec2(50, 180), &s3, 0, 127, s3_label)) {
                hardware->simulateSliderPot(2, s3);
                if (prev_s3 != s3) {
                    char log_msg[128];
                    snprintf(log_msg, sizeof(log_msg), "Mode %d, Track %d, Step %d: %s changed to %d",
                             engine->getCurrentMode(), engine->getCurrentTrack(), engine->getCurrentStep() + 1,
                             GetSliderLabel(2, current_mode), s3);
                    hardware->addLog(log_msg);
                    prev_s3 = s3;
                }
            }
            ImGui::SameLine();
            if (ImGui::VSliderInt("##S4", ImVec2(50, 180), &s4, 0, 127, s4_label)) {
                hardware->simulateSliderPot(3, s4);
                if (prev_s4 != s4) {
                    char log_msg[128];
                    snprintf(log_msg, sizeof(log_msg), "Mode %d, Track %d, Step %d: %s changed to %d",
                             engine->getCurrentMode(), engine->getCurrentTrack(), engine->getCurrentStep() + 1,
                             GetSliderLabel(3, current_mode), s4);
                    hardware->addLog(log_msg);
                    prev_s4 = s4;
                }
            }

            ImGui::Separator();

            // Pattern grid visualization
            ImGui::Text("Pattern Grid (Track %d)", engine->getCurrentTrack());
            Mode& editing_mode = song->getMode(engine->getCurrentMode());
            Pattern& current_pattern = editing_mode.getPattern(engine->getCurrentPattern());
            Track& current_track = current_pattern.getTrack(engine->getCurrentTrack());

            for (int step = 0; step < 16; step++) {
                const Event& evt = current_track.getEvent(step);
                bool has_event = evt.getSwitch();

                ImVec4 color = has_event ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
                if (step == engine->getCurrentStep()) {
                    color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red for current step
                }

                ImGui::PushStyleColor(ImGuiCol_Button, color);
                char btn_label[8];
                snprintf(btn_label, sizeof(btn_label), "%d", step + 1);

                if (ImGui::Button(btn_label, ImVec2(40, 40))) {
                    // Click to toggle event
                    Event& evt_mut = current_track.getEvent(step);
                    bool new_state = !evt_mut.getSwitch();
                    evt_mut.setSwitch(new_state);

                    // Log the change
                    char log_msg[128];
                    snprintf(log_msg, sizeof(log_msg),
                             "Mode %d, Track %d, Step %d: Switch %s (S1=%d S2=%d S3=%d S4=%d)",
                             engine->getCurrentMode(), engine->getCurrentTrack(), step + 1,
                             new_state ? "ON" : "OFF",
                             evt_mut.getPot(0), evt_mut.getPot(1), evt_mut.getPot(2), evt_mut.getPot(3));
                    hardware->addLog(log_msg);
                }
                ImGui::PopStyleColor();

                if (step < 15) ImGui::SameLine();
                if (step == 7) ImGui::NewLine();
            }

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
