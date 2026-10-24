#include <luma/app/Application.hpp>
#include <luma/core/GlfwWindow.hpp>
#include <luma/editor/EditorUI.hpp>
#include <luma/gfx/NativeResourceFactory.hpp>
#include <luma/gfx/RenderState.hpp>
#include <luma/render/Renderer.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace luma {
namespace {
void screenshot(const std::filesystem::path& path, int width, int height) {
    if (width <= 0 || height <= 0)
        throw std::runtime_error("cannot capture zero-sized framebuffer");
    std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glReadBuffer(GL_BACK);
    GLint alignment;
    glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glPixelStorei(GL_PACK_ALIGNMENT, alignment);
    checkGl("screenshot readback");
    if (path.has_parent_path())
        std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    if (!out)
        throw std::runtime_error("cannot write screenshot " + path.string());
    out << "P6\n" << width << ' ' << height << "\n255\n";
    for (int y = height - 1; y >= 0; --y)
        out.write(reinterpret_cast<const char*>(pixels.data() + static_cast<size_t>(y) * width * 3),
                  width * 3);
    if (!out)
        throw std::runtime_error("screenshot write failed " + path.string());
    std::cout << "Screenshot: " << path << " (P6 PPM, " << width << 'x' << height << ")\n";
}
} // namespace
Application::Application(std::string title, int w, int h)
    : title_(std::move(title)), width_(w), height_(h) {
    if (w <= 0 || h <= 0)
        throw std::invalid_argument("logical window dimensions must be positive");
}
int Application::run(std::unique_ptr<SceneLayer> layer, const RunOptions& options) {
    if (ran_ || !layer) {
        std::cerr << "Application requires a nonnull layer and may run once\n";
        return 1;
    }
    ran_ = true;
    try {
        GlfwRuntime runtime;
        GlfwWindow window(title_, width_, height_, options.hidden);
        // Locals are destroyed in reverse order, before the window/context.
        NativeResourceFactory resources;
        AssetStore assets(resources);
        Scene scene;
        Camera camera;
        CameraControls controls;
        EditorModel model(scene, assets.materials(), camera, controls);
        EditorController controller(model);
        Renderer renderer(resources);
        renderer.initialize(assetRoot() / "shaders");
        SceneContext context{assets, scene, camera, controls, model.lights(), model.lighting(),
                             model.postProcess()};
        double scroll = 0;
        glfwSetWindowUserPointer(window.native(), &scroll);
        glfwSetScrollCallback(window.native(), [](GLFWwindow* w, double, double y) {
            *static_cast<double*>(glfwGetWindowUserPointer(w)) += y;
        });
        std::unique_ptr<EditorUI> editor;
        if (options.editor)
            editor = std::make_unique<EditorUI>(window.native());
        InputRouter inputs;
        bool created = false, destroyFailed = false;
        auto destroy = [&] {
            if (created) {
                created = false;
                try {
                    layer->onDestroy(context);
                } catch (const std::exception& e) {
                    destroyFailed = true;
                    std::cerr << "onDestroy: " << e.what() << '\n';
                } catch (...) {
                    destroyFailed = true;
                    std::cerr << "onDestroy: unknown exception\n";
                }
            }
        };
        try {
            layer->onCreate(context);
            created = true;
            using Clock = std::chrono::steady_clock;
            auto last = Clock::now();
            double elapsed = 0;
            int frame = 0;
            while (!window.shouldClose() && (options.frames <= 0 || frame < options.frames)) {
                window.poll();
                if (glfwGetKey(window.native(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
                    glfwSetWindowShouldClose(window.native(), GL_TRUE);
                if (editor)
                    editor->beginFrame();
                auto now = Clock::now();
                double wallDelta = std::chrono::duration<double>(now - last).count();
                last = now;
                float delta = options.deterministic
                                  ? 1.0f / 60
                                  : static_cast<float>(std::clamp(wallDelta, 0.0, 0.05));
                elapsed += delta;
                auto input = inputs.poll(window.native(), editor && editor->captureMouse(),
                                         editor && editor->captureKeyboard());
                if (input.focused && !(editor && editor->captureMouse()))
                    input.scroll = scroll;
                scroll = 0;
                controls.update(camera, input, delta);
                layer->onUpdate(context, {delta, elapsed, input});
                std::string requested;
                controller.reconcileSelection();
                if (editor)
                    requested = editor->inspect(model, controller,
                                                options.deterministic ? delta : wallDelta);
                if (!requested.empty()) {
                    destroy();
                    scene.clear();
                    layer = makePreset(requested);
                    layer->onCreate(context);
                    created = true;
                    controller.reconcileSelection();
                }
                renderer.setStyle(model.style());
                renderer.options() = model.options();
                renderer.setLighting(model.lighting(), model.lights());
                renderer.setPostProcess(model.postProcess());
                int w, h;
                window.framebufferSize(w, h);
                if (w > 0 && h > 0) {
                    camera.setAspect(static_cast<float>(w) / h);
                    renderer.render(scene, assets, camera, {0, w, h, window.srgbAttachment()});
                    if (editor)
                        editor->draw();
                    checkGl("complete scene + editor frame");
                    if (!options.screenshot.empty() &&
                        ((options.frames > 0 && frame == options.frames - 1) ||
                         window.shouldClose()))
                        screenshot(options.screenshot, w, h);
                    window.swap();
                } else {
                    if (editor) {
                        editor->endWithoutDraw();
                    }
                    glfwWaitEventsTimeout(0.05);
                    continue;
                }
                ++frame;
            }
            destroy();
            if (destroyFailed)
                return 1;
            std::cout << "Completed " << frame << " frames; no OpenGL errors\n";
        } catch (...) {
            destroy();
            throw;
        }
        glfwSetWindowUserPointer(window.native(), nullptr);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Application: " << e.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "Application: unknown failure\n";
        return 1;
    }
}
} // namespace luma
