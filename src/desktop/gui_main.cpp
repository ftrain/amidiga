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
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

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

    // Enable external MIDI by default (matches GUI checkbox default)
    engine->setUseExternalMIDI(true);

    // Mode 0: Song/Pattern Sequencer - Default pattern chain
    // Patterns 0-3 repeating 4 times each (steps 0-3, 4-7, 8-11, 12-15)
    Mode& mode0 = song->getMode(0);
    Pattern& song_pattern = mode0.getPattern(0);  // Mode 0 always uses pattern 0

    // All steps: Pattern 0 (where we put the demo content)
    // S1 encoding: value * 32 / 128 = pattern number (0-31)
    // So S1 = 0 means pattern 0
    for (int step = 0; step < 16; step++) {
        Event& event = song_pattern.getEvent(0, step);
        event.setSwitch(true);
        event.setPot(0, 0);  // S1 = 0 -> Pattern 0 (displayed as pattern 1)
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

        // Main GUI window with tabs
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(1280, 720), ImGuiCond_FirstUseEver);
            ImGui::Begin("GRUVBOK", nullptr, ImGuiWindowFlags_NoCollapse);

            if (ImGui::BeginTabBar("MainTabs", ImGuiTabBarFlags_None)) {
                // Tab 1: Hardware Controls
                if (ImGui::BeginTabItem("Hardware")) {

            // MIDI Output Port Selector
            ImGui::Text("MIDI Output:");
            ImGui::SameLine();

            int port_count = hardware->getMidiPortCount();
            int current_port = hardware->getCurrentMidiPort();

            std::string preview;
            if (current_port < 0) {
                preview = "Virtual Port";
            } else {
                preview = hardware->getMidiPortName(current_port);
                if (preview.empty()) {
                    preview = "Port " + std::to_string(current_port);
                }
            }
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

            // Audio Output Section
            ImGui::Separator();
            ImGui::Text("Audio Output");

            // Internal Audio checkbox
            static bool internal_audio_enabled = false;
            static bool audio_initialized = false;
            static std::string loaded_soundfont = "";

            if (ImGui::Checkbox("Internal Audio (FluidSynth)", &internal_audio_enabled)) {
                if (internal_audio_enabled && !audio_initialized) {
                    // Try to initialize audio on first enable
                    // Look for SoundFont in common locations
                    std::vector<std::string> soundfont_paths = {
                        "/opt/homebrew/Cellar/fluid-synth/2.5.1/share/soundfonts/default.sf2",  // Homebrew default
                        "/opt/homebrew/share/soundfonts/default.sf2",  // Homebrew symlink
                        "/usr/share/soundfonts/FluidR3_GM.sf2",
                        "/usr/share/sounds/sf2/FluidR3_GM.sf2",
                        "/usr/local/share/soundfonts/FluidR3_GM.sf2",
                        "FluidR3_GM.sf2",  // Current directory
                    };

                    bool found = false;
                    for (const auto& path : soundfont_paths) {
                        if (engine->initAudioOutput(path)) {
                            audio_initialized = true;
                            found = true;
                            loaded_soundfont = path;
                            hardware->addLog("[Audio] Initialized with SoundFont: " + path);
                            break;
                        }
                    }

                    if (!found) {
                        hardware->addLog("[Audio] ERROR: No SoundFont found in default locations");
                        hardware->addLog("[Audio] Download a .sf2 file to the current directory or /opt/homebrew/share/soundfonts/");
                        internal_audio_enabled = false;
                    }
                }

                engine->setUseInternalAudio(internal_audio_enabled);
            }

            if (engine->isAudioOutputReady()) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[READY]");

                // Show loaded soundfont (just filename)
                if (!loaded_soundfont.empty()) {
                    size_t last_slash = loaded_soundfont.find_last_of("/\\");
                    std::string sf_name = (last_slash != std::string::npos) ?
                        loaded_soundfont.substr(last_slash + 1) : loaded_soundfont;
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(%s)", sf_name.c_str());
                }

                // Gain control
                ImGui::PushItemWidth(150);
                static float gain = 0.5f;
                if (ImGui::SliderFloat("Volume", &gain, 0.0f, 2.0f, "%.2f")) {
                    engine->setAudioGain(gain);
                }
                ImGui::PopItemWidth();
            } else if (internal_audio_enabled && !audio_initialized) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[FAILED - check logs]");
            }

            // External MIDI checkbox
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::SameLine();
            static bool external_midi_enabled = true;  // Default ON
            if (ImGui::Checkbox("External MIDI", &external_midi_enabled)) {
                engine->setUseExternalMIDI(external_midi_enabled);
            }

            // Save/Load Section
            ImGui::Separator();
            ImGui::Text("Song Persistence");

            // Static state for UI persistence
            static char song_name_buf[128] = "My GRUVBOK Song";
            static char save_path_buf[256] = "/tmp/gruvbok_song.json";
            static char load_path_buf[256] = "/tmp/gruvbok_song.json";

            // Song name input
            ImGui::PushItemWidth(300);
            ImGui::InputText("Song Name", song_name_buf, sizeof(song_name_buf));
            ImGui::PopItemWidth();

            // Save section
            ImGui::PushItemWidth(400);
            ImGui::InputText("Save Path", save_path_buf, sizeof(save_path_buf));
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("Save")) {
                int current_tempo = engine->getTempo();
                if (song->save(save_path_buf, song_name_buf, current_tempo)) {
                    hardware->addLog("✓ Song saved: " + std::string(save_path_buf));
                    engine->triggerLEDPattern(Engine::LEDPattern::SAVING);
                } else {
                    hardware->addLog("✗ ERROR: Failed to save song");
                    engine->triggerLEDPattern(Engine::LEDPattern::ERROR);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Quick Save")) {
                // Use last path with timestamp
                std::string quick_path = "/tmp/gruvbok_autosave_" + std::to_string(hardware->getMillis()) + ".json";
                int current_tempo = engine->getTempo();
                if (song->save(quick_path, song_name_buf, current_tempo)) {
                    hardware->addLog("✓ Autosaved: " + quick_path);
                    engine->triggerLEDPattern(Engine::LEDPattern::SAVING);
                    // Update save path to autosave location
                    snprintf(save_path_buf, sizeof(save_path_buf), "%s", quick_path.c_str());
                } else {
                    hardware->addLog("✗ ERROR: Autosave failed");
                    engine->triggerLEDPattern(Engine::LEDPattern::ERROR);
                }
            }

            // Load section
            ImGui::PushItemWidth(400);
            ImGui::InputText("Load Path", load_path_buf, sizeof(load_path_buf));
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("Load")) {
                engine->triggerLEDPattern(Engine::LEDPattern::LOADING);
                std::string loaded_name;
                int loaded_tempo = 120;
                if (song->load(load_path_buf, &loaded_name, &loaded_tempo)) {
                    hardware->addLog("✓ Song loaded: " + std::string(load_path_buf));
                    hardware->addLog("  Name: " + loaded_name + ", Tempo: " + std::to_string(loaded_tempo) + " BPM");

                    // Update UI with loaded metadata
                    snprintf(song_name_buf, sizeof(song_name_buf), "%s", loaded_name.c_str());

                    // Apply loaded tempo to engine
                    engine->setTempo(loaded_tempo);

                    // Update save path to match load path (for easy resave)
                    snprintf(save_path_buf, sizeof(save_path_buf), "%s", load_path_buf);

                    // Return to tempo beat pattern after successful load
                    engine->triggerLEDPattern(Engine::LEDPattern::TEMPO_BEAT);
                } else {
                    hardware->addLog("✗ ERROR: Failed to load from " + std::string(load_path_buf));
                    engine->triggerLEDPattern(Engine::LEDPattern::ERROR);
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

            // Controls section: 2x2 knobs on left, sliders on right
            ImGui::BeginGroup();

            // Left side: Global controls (R1-R4) as 2x2 grid of knobs
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
            snprintf(pattern_str, sizeof(pattern_str), "%d", pattern_val + 1);  // Display as 1-32
            snprintf(track_str, sizeof(track_str), "%d", track_val + 1);        // Display as 1-8

            // Row 1: Mode and Tempo
            if (Knob("Mode", &r1, 0, 127, mode_str, 30.0f)) hardware->simulateRotaryPot(0, r1);
            ImGui::SameLine(0, 10);
            if (Knob("Tempo", &r2, 0, 127, tempo_str, 30.0f)) hardware->simulateRotaryPot(1, r2);

            // Row 2: Pattern and Track
            if (Knob("Pattern", &r3, 0, 127, pattern_str, 30.0f)) hardware->simulateRotaryPot(2, r3);
            ImGui::SameLine(0, 10);
            if (Knob("Track", &r4, 0, 127, track_str, 30.0f)) hardware->simulateRotaryPot(3, r4);

            ImGui::EndGroup();

            // Right side: Slider pots (S1-S4) with mode-specific labels
            ImGui::SameLine(0, 20);
            ImGui::BeginGroup();

            int current_mode = engine->getCurrentMode();
            LuaContext* lua_mode = mode_loader->getMode(current_mode);
            std::string mode_name = lua_mode && lua_mode->isValid() ? lua_mode->getModeName() : "Unknown";
            ImGui::Text("Mode %d: %s", current_mode, mode_name.c_str());

            int s1 = hardware->readSliderPot(0);
            int s2 = hardware->readSliderPot(1);
            int s3 = hardware->readSliderPot(2);
            int s4 = hardware->readSliderPot(3);

            char s1_label[64], s2_label[64], s3_label[64], s4_label[64];
            // For Mode 0, S1 represents pattern number (1-32), not raw MIDI value
            if (current_mode == 0) {
                int pattern_num = ((s1 * 32) / 128) + 1;  // Convert 0-127 to 1-32
                snprintf(s1_label, sizeof(s1_label), "%s\n%d", GetSliderLabel(0, current_mode), pattern_num);
            } else {
                snprintf(s1_label, sizeof(s1_label), "%s\n%d", GetSliderLabel(0, current_mode), s1);
            }
            snprintf(s2_label, sizeof(s2_label), "%s\n%d", GetSliderLabel(1, current_mode), s2);
            snprintf(s3_label, sizeof(s3_label), "%s\n%d", GetSliderLabel(2, current_mode), s3);
            snprintf(s4_label, sizeof(s4_label), "%s\n%d", GetSliderLabel(3, current_mode), s4);

            // Sliders now only set values when you press a button (parameter lock)
            if (ImGui::VSliderInt("##S1", ImVec2(40, 140), &s1, 0, 127, s1_label)) {
                hardware->simulateSliderPot(0, s1);
            }
            ImGui::SameLine(0, 8);
            if (ImGui::VSliderInt("##S2", ImVec2(40, 140), &s2, 0, 127, s2_label)) {
                hardware->simulateSliderPot(1, s2);
            }
            ImGui::SameLine(0, 8);
            if (ImGui::VSliderInt("##S3", ImVec2(40, 140), &s3, 0, 127, s3_label)) {
                hardware->simulateSliderPot(2, s3);
            }
            ImGui::SameLine(0, 8);
            if (ImGui::VSliderInt("##S4", ImVec2(40, 140), &s4, 0, 127, s4_label)) {
                hardware->simulateSliderPot(3, s4);
            }

            ImGui::EndGroup();
            ImGui::EndGroup();
            ImGui::Separator();

            // Pattern grid visualization
            ImGui::Text("Pattern Grid (Track %d)", engine->getCurrentTrack() + 1);  // Display as 1-8
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

                // Mode 0 runs at 1/16th speed, so show song_mode_step_ instead of current_step_
                int display_step = (engine->getCurrentMode() == 0) ? engine->getSongModeStep() : engine->getCurrentStep();

                if (held_button == step) {
                    color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); // Yellow for held button
                } else if (step == display_step) {
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
                    ImGui::EndTabItem();
                }

                // Tab 2: Song Data Explorer
                if (ImGui::BeginTabItem("Song Data")) {

            ImGui::Text("Navigate the entire song structure");
            ImGui::Separator();

            // Sync explorer position with engine's current position
            int explorer_mode = engine->getCurrentMode();
            int explorer_pattern = engine->getCurrentPattern();
            int explorer_track = engine->getCurrentTrack();

            ImGui::Text("Current Position (updates with knobs):");
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Mode: %d  Pattern: %d  Track: %d",
                             explorer_mode, explorer_pattern + 1, explorer_track + 1);  // Display patterns as 1-32, tracks as 1-8

            ImGui::Separator();

            // Show events for selected mode/pattern/track in a table
            Mode& exp_mode = song->getMode(explorer_mode);
            Pattern& exp_pattern = exp_mode.getPattern(explorer_pattern);
            Track& exp_track = exp_pattern.getTrack(explorer_track);

            ImGui::Text("Events: Mode %d, Pattern %d, Track %d", explorer_mode, explorer_pattern + 1, explorer_track + 1);  // Display patterns as 1-32, tracks as 1-8

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
                    // Mode 0 runs at 1/16th speed, so compare with song_mode_step_ instead of current_step_
                    int explorer_display_step = (explorer_mode == 0) ? engine->getSongModeStep() : engine->getCurrentStep();

                    bool is_current = (explorer_mode == engine->getCurrentMode() &&
                                      explorer_pattern == engine->getCurrentPattern() &&
                                      explorer_track == engine->getCurrentTrack() &&
                                      step == explorer_display_step);
                    if (is_current) {
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(0.3f, 0.3f, 0.6f, 0.3f)));
                    }

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", step + 1);

                    ImGui::TableNextColumn();
                    ImGui::Text("%s", evt.getSwitch() ? "X" : "");

                    ImGui::TableNextColumn();
                    // For Mode 0, S1 represents pattern number (1-32), not raw MIDI value
                    if (explorer_mode == 0) {
                        int pattern_num = ((evt.getPot(0) * 32) / 128) + 1;  // Convert 0-127 to 1-32
                        ImGui::Text("%d", pattern_num);
                    } else {
                        ImGui::Text("%d", evt.getPot(0));
                    }

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

                    ImGui::EndTabItem();
                }

                // Tab 3: System Log
                if (ImGui::BeginTabItem("Log")) {

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
                    ImGui::EndTabItem();
                }

                // Tab 4: Lua Mode Editor
                if (ImGui::BeginTabItem("Editor")) {

            // Static state for editor
            static int selected_mode = 1;  // Default to mode 1 (drums)
            static char lua_code_buffer[32768] = "";  // 32KB buffer for Lua code
            static bool file_loaded = false;
            static std::string current_filename = "";
            static std::string last_saved_filename = "";

            // Mode selector
            ImGui::Text("Edit Mode:");
            ImGui::SameLine();
            ImGui::PushItemWidth(150);
            if (ImGui::Combo("##ModeSelect", &selected_mode,
                "Mode 0 (Song)\0Mode 1 (Drums)\0Mode 2 (Acid)\0Mode 3 (Chords)\0Mode 4\0Mode 5\0Mode 6\0Mode 7\0"
                "Mode 8\0Mode 9\0Mode 10\0Mode 11\0Mode 12\0Mode 13\0Mode 14\0\0")) {
                // Mode changed - load new file
                file_loaded = false;
            }
            ImGui::PopItemWidth();

            // Show current filename
            char mode_filename[64];
            snprintf(mode_filename, sizeof(mode_filename), "modes/%02d_*.lua", selected_mode);
            current_filename = mode_filename;

            // Load button
            ImGui::SameLine();
            if (ImGui::Button("Load from Disk") || !file_loaded) {
                // Find any mode file matching NN_*.lua pattern
                std::string mode_path;
                bool found_file = false;

                // Search for files matching the pattern
                char pattern[64];
                snprintf(pattern, sizeof(pattern), "modes/%02d_", selected_mode);
                std::string pattern_str(pattern);

                // Use filesystem to find matching files
                namespace fs = std::filesystem;
                if (fs::exists("modes") && fs::is_directory("modes")) {
                    for (const auto& entry : fs::directory_iterator("modes")) {
                        if (entry.is_regular_file()) {
                            std::string filename = entry.path().filename().string();
                            // Check if filename starts with "NN_" and ends with ".lua"
                            if (filename.find(pattern_str.substr(6)) == 0 &&
                                filename.size() >= 4 && filename.substr(filename.size() - 4) == ".lua") {
                                mode_path = entry.path().string();
                                found_file = true;
                                break;  // Use first matching file
                            }
                        }
                    }
                }

                if (found_file) {
                    // Try to open and read the file
                    std::ifstream file(mode_path);
                    if (file.is_open()) {
                        std::stringstream buffer;
                        buffer << file.rdbuf();
                        std::string content = buffer.str();

                        // Copy to buffer (with safety check)
                        size_t copy_size = std::min(content.size(), sizeof(lua_code_buffer) - 1);
                        memcpy(lua_code_buffer, content.c_str(), copy_size);
                        lua_code_buffer[copy_size] = '\0';

                        file_loaded = true;
                        current_filename = mode_path;
                        last_saved_filename = mode_path;
                        hardware->addLog("[Editor] Loaded: " + mode_path);
                    }
                } else {
                    // File doesn't exist - create template
                    const char* template_code =
                        "-- GRUVBOK Lua Mode Template\n"
                        "-- Mode: %d\n\n"
                        "MODE_NAME = \"Mode %d\"\n\n"
                        "function init(context)\n"
                        "    -- Called once when mode loads\n"
                        "    print(\"Mode %d initialized on channel \" .. context.midi_channel)\n"
                        "end\n\n"
                        "function process_event(track, event)\n"
                        "    -- Called for each event during playback\n"
                        "    -- track: 0-7\n"
                        "    -- event.switch: true/false\n"
                        "    -- event.pots: {s1, s2, s3, s4} (0-127 each)\n\n"
                        "    if event.switch then\n"
                        "        -- Send MIDI note\n"
                        "        note(60, 100)  -- Middle C, velocity 100\n"
                        "        off(60, 100)   -- Note off after 100ms\n"
                        "    end\n\n"
                        "    return {}\n"
                        "end\n";

                    snprintf(lua_code_buffer, sizeof(lua_code_buffer), template_code,
                             selected_mode, selected_mode, selected_mode);
                    file_loaded = true;
                    current_filename = ""; // No file yet
                    hardware->addLog("[Editor] No file found for mode " + std::to_string(selected_mode) + " - showing template");
                }
            }

            // Save button
            ImGui::SameLine();
            if (ImGui::Button("Save to Disk")) {
                // Write buffer to file
                std::ofstream file(current_filename);
                if (file.is_open()) {
                    file << lua_code_buffer;
                    file.close();
                    last_saved_filename = current_filename;
                    hardware->addLog("[Editor] Saved: " + current_filename);
                } else {
                    hardware->addLog("[Editor] ERROR: Failed to save " + current_filename);
                }
            }

            // Hot-reload button
            ImGui::SameLine();
            if (ImGui::Button("Hot Reload Mode")) {
                // Save first, then reload
                std::ofstream file(current_filename);
                if (file.is_open()) {
                    file << lua_code_buffer;
                    file.close();

                    // Reload the mode
                    LuaContext* lua_mode = mode_loader->getMode(selected_mode);
                    if (lua_mode) {
                        if (lua_mode->loadScript(current_filename)) {
                            // Reinit with current tempo
                            LuaInitContext context;
                            context.tempo = engine->getTempo();
                            context.mode_number = selected_mode;
                            context.midi_channel = selected_mode;
                            lua_mode->callInit(context);

                            hardware->addLog("[Editor] Hot-reloaded mode " + std::to_string(selected_mode));
                        } else {
                            hardware->addLog("[Editor] ERROR: Failed to reload mode " + std::to_string(selected_mode));
                        }
                    }
                } else {
                    hardware->addLog("[Editor] ERROR: Failed to save before reload");
                }
            }

            ImGui::Separator();
            ImGui::Text("File: %s", current_filename.c_str());
            ImGui::Separator();

            // Lua code editor (monospace font, large text area)
            ImGui::PushFont(io.Fonts->Fonts[0]);  // Use default monospace-ish font
            ImGui::InputTextMultiline("##LuaCode", lua_code_buffer, sizeof(lua_code_buffer),
                ImVec2(-1, -1),
                ImGuiInputTextFlags_AllowTabInput);
            ImGui::PopFont();

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
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
