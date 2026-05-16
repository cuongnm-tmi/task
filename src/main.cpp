#include "StoreApp.h"

#include <filesystem>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

iconmode::AppConfig parseArgs(int argc, char** argv) {
    iconmode::AppConfig config;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if ((arg == "--model" || arg == "-m") && i + 1 < argc) {
            config.modelPath = argv[++i];
        } else if (arg == "--vulkan") {
            config.backend = filament::Engine::Backend::VULKAN;
        } else if (arg == "--opengl") {
            config.backend = filament::Engine::Backend::OPENGL;
        } else if ((arg == "--help" || arg == "-h")) {
            std::cout
                << "ICON MODE Store Filament viewer\n"
                << "Usage: icon_mode_store [--model assets/models/icon_mode_store.glb] [--opengl|--vulkan]\n"
                << "Mouse: hold right button to look. Keys: WASD move, Q/E down/up, Esc quit.\n";
            std::exit(0);
        }
    }

    return config;
}

} // namespace

int main(int argc, char** argv) {
    try {
        auto config = parseArgs(argc, argv);
        iconmode::StoreApp app(std::move(config));
        app.run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[icon_mode_store] " << error.what() << '\n';
        return 1;
    }
}
