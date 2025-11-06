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

using namespace gruvbok;

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
    std::cout << "=== GRUVBOK GUI Simulator ===" << std::endl;

    auto hardware = std::make_unique<DesktopHardware>();
    if (!hardware->init()) {
        std::cerr << "Failed to initialize hardware" << std::endl;
        return 1;
    }

    auto song = std::make_unique<Song>();
    auto mode_loader = std::make_unique<ModeLoader>();

    std::cout << "Loading modes..." << std::endl;
    int loaded = mode_loader->loadModesFromDirectory("modes", 120);
    if (loaded == 0) {
        std::cerr << "Warning: No modes loaded" << std::endl;
    }

    auto engine = std::make_unique<Engine>(song.get(), hardware.get(), mode_loader.get());

    // Create test pattern
    std::cout << "Creating test pattern..." << std::endl;
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

    engine->start();
    std::cout << "Engine started!" << std::endl;

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
            ImGui::Text("| Tempo: %d BPM", engine->getTempo());
            ImGui::SameLine();
            ImGui::Text("| Mode: %d", engine->getCurrentMode());
            ImGui::SameLine();
            ImGui::Text("| Pattern: %d", engine->getCurrentPattern());
            ImGui::SameLine();
            ImGui::Text("| Track: %d", engine->getCurrentTrack());
            ImGui::SameLine();
            ImGui::Text("| Step: %d", engine->getCurrentStep());

            ImGui::Separator();

            // Global controls (R1-R4)
            ImGui::Text("Global Controls (Rotary Pots)");

            int r1 = hardware->readRotaryPot(0);
            int r2 = hardware->readRotaryPot(1);
            int r3 = hardware->readRotaryPot(2);
            int r4 = hardware->readRotaryPot(3);

            if (ImGui::SliderInt("R1 Mode", &r1, 0, 127)) hardware->simulateRotaryPot(0, r1);
            if (ImGui::SliderInt("R2 Tempo", &r2, 0, 127)) hardware->simulateRotaryPot(1, r2);
            if (ImGui::SliderInt("R3 Pattern", &r3, 0, 127)) hardware->simulateRotaryPot(2, r3);
            if (ImGui::SliderInt("R4 Track", &r4, 0, 127)) hardware->simulateRotaryPot(3, r4);

            ImGui::Separator();

            // Button grid (B1-B16)
            ImGui::Text("Step Buttons (B1-B16)");
            for (int row = 0; row < 2; row++) {
                for (int col = 0; col < 8; col++) {
                    int btn = row * 8 + col;
                    char label[16];
                    snprintf(label, sizeof(label), "B%d", btn + 1);

                    bool pressed = hardware->readButton(btn);
                    if (ImGui::Checkbox(label, &pressed)) {
                        hardware->simulateButton(btn, pressed);
                    }

                    if (col < 7) ImGui::SameLine();
                }
            }

            ImGui::Separator();

            // Slider pots (S1-S4)
            ImGui::Text("Slider Pots (S1-S4)");

            int s1 = hardware->readSliderPot(0);
            int s2 = hardware->readSliderPot(1);
            int s3 = hardware->readSliderPot(2);
            int s4 = hardware->readSliderPot(3);

            if (ImGui::VSliderInt("##S1", ImVec2(40, 160), &s1, 0, 127, "S1\n%d")) {
                hardware->simulateSliderPot(0, s1);
            }
            ImGui::SameLine();
            if (ImGui::VSliderInt("##S2", ImVec2(40, 160), &s2, 0, 127, "S2\n%d")) {
                hardware->simulateSliderPot(1, s2);
            }
            ImGui::SameLine();
            if (ImGui::VSliderInt("##S3", ImVec2(40, 160), &s3, 0, 127, "S3\n%d")) {
                hardware->simulateSliderPot(2, s3);
            }
            ImGui::SameLine();
            if (ImGui::VSliderInt("##S4", ImVec2(40, 160), &s4, 0, 127, "S4\n%d")) {
                hardware->simulateSliderPot(3, s4);
            }

            ImGui::Separator();

            // Pattern grid visualization
            ImGui::Text("Pattern Grid (Track %d)", engine->getCurrentTrack());
            Mode& current_mode = song->getMode(engine->getCurrentMode());
            Pattern& current_pattern = current_mode.getPattern(engine->getCurrentPattern());
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
                    evt_mut.setSwitch(!evt_mut.getSwitch());
                }
                ImGui::PopStyleColor();

                if (step < 15) ImGui::SameLine();
                if (step == 7) ImGui::NewLine();
            }

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
    std::cout << "\nShutting down..." << std::endl;
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
