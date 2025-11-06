#include "mode_loader.h"
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace gruvbok {

ModeLoader::ModeLoader() {
    // Initialize with nullptrs
    for (auto& mode : modes_) {
        mode = nullptr;
    }
}

bool ModeLoader::loadMode(int mode_number, const std::string& filepath, int tempo) {
    if (mode_number < 0 || mode_number >= NUM_MODES) {
        std::cerr << "Invalid mode number: " << mode_number << std::endl;
        return false;
    }

    auto context = std::make_unique<LuaContext>();

    if (!context->loadScript(filepath)) {
        std::cerr << "Failed to load mode " << mode_number << ": " << context->getError() << std::endl;
        return false;
    }

    // Set the MIDI channel (mode number = channel)
    context->setChannel(static_cast<uint8_t>(mode_number));

    // Call init
    LuaInitContext init_ctx;
    init_ctx.tempo = tempo;
    init_ctx.mode_number = mode_number;
    init_ctx.midi_channel = mode_number;

    if (!context->callInit(init_ctx)) {
        std::cerr << "Failed to initialize mode " << mode_number << ": " << context->getError() << std::endl;
        return false;
    }

    modes_[mode_number] = std::move(context);
    std::cout << "Loaded mode " << mode_number << " from " << filepath << std::endl;
    return true;
}

LuaContext* ModeLoader::getMode(int mode_number) {
    if (mode_number < 0 || mode_number >= NUM_MODES) {
        return nullptr;
    }
    return modes_[mode_number].get();
}

bool ModeLoader::isModeLoaded(int mode_number) const {
    if (mode_number < 0 || mode_number >= NUM_MODES) {
        return false;
    }
    return modes_[mode_number] != nullptr && modes_[mode_number]->isValid();
}

int ModeLoader::loadModesFromDirectory(const std::string& directory, int tempo) {
    namespace fs = std::filesystem;

    if (!fs::exists(directory) || !fs::is_directory(directory)) {
        std::cerr << "Directory does not exist: " << directory << std::endl;
        return 0;
    }

    int loaded_count = 0;

    // Look for files matching pattern: NN_*.lua where NN is 00-14
    for (const auto& entry : fs::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".lua") {
            std::string filename = entry.path().filename().string();

            // Try to parse mode number from filename (e.g., "01_drums.lua" -> 1)
            if (filename.length() >= 2 && std::isdigit(filename[0]) && std::isdigit(filename[1])) {
                int mode_number = std::stoi(filename.substr(0, 2));

                if (mode_number >= 0 && mode_number < NUM_MODES) {
                    if (loadMode(mode_number, entry.path().string(), tempo)) {
                        loaded_count++;
                    }
                }
            }
        }
    }

    std::cout << "Loaded " << loaded_count << " modes from " << directory << std::endl;
    return loaded_count;
}

} // namespace gruvbok
