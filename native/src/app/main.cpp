#include <iostream>
#include <luma/app/Application.hpp>
#include <stdexcept>
#include <string>
int main(int argc, char** argv) {
    try {
        luma::RunOptions options;
        std::string preset = "robot";
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            auto value = [&]() {
                if (++i >= argc)
                    throw std::invalid_argument("missing value after " + arg);
                return std::string(argv[i]);
            };
            if (arg == "--help") {
                std::cout << "Luma: OpenGL 3.3 stage-one/two workbench\nUsage: luma_workbench "
                             "[--example window|triangle|quad|texture|camera|state]\n [--preset "
                             "robot|quad|texture|state|lighting] [--frames N] [--hidden] [--no-ui]\n "
                             "[--screenshot PATH.ppm] [--deterministic]\nFinite --frames uses "
                             "deterministic 1/60s steps. Screenshot is P6 PPM.\nOrbit: LMB "
                             "drag/wheel. Fly: RMB drag + WASD, Q/E, Shift. Esc exits.\nRun "
                             "luma_gl_tests --hidden for real OpenGL integration checks.\n";
                return 0;
            } else if (arg == "--example" || arg == "--preset")
                preset = value();
            else if (arg == "--frames") {
                auto v = value();
                size_t end;
                options.frames = std::stoi(v, &end);
                if (end != v.size() || options.frames <= 0)
                    throw std::invalid_argument("--frames must be a positive integer");
                options.deterministic = true;
            } else if (arg == "--hidden")
                options.hidden = true;
            else if (arg == "--no-ui")
                options.editor = false;
            else if (arg == "--deterministic")
                options.deterministic = true;
            else if (arg == "--screenshot")
                options.screenshot = value();
            else
                throw std::invalid_argument("unknown argument: " + arg);
        }
        if (!options.screenshot.empty() && options.frames == 0) {
            options.frames = 1;
            options.deterministic = true;
        }
        luma::Application app("Luma / OpenGL Workbench", 1280, 800);
        return app.run(luma::makePreset(preset), options);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
