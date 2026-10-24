#include <luma/gfx/RenderState.hpp>
#include <luma/render/PostProcessPipeline.hpp>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace {
void check(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
std::array<unsigned char, 4> pixel(int x, int y) {
    std::array<unsigned char, 4> value{};
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, value.data());
    return value;
}
}
void postProcessChecks(luma::IResourceFactory& resources, const std::filesystem::path& shaders) {
    using namespace luma;
    PostProcessPipeline pipeline;
    pipeline.initialize(resources, shaders);
    PostProcessTarget source, scratch, output;
    source.resize(16, 16);
    output.resize(16, 16);
    output.bindForScene();
    check(source.complete(), "FBO completeness must test the owned target");
    std::vector<unsigned char> data(16 * 16 * 4, 255);
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x) {
            auto i = 4 * (y * 16 + x);
            data[i] = y < 8 ? 64 : 192;
            data[i + 1] = x < 8 ? 32 : 160;
            data[i + 2] = 96;
        }
    glBindTexture(GL_TEXTURE_2D, source.texture());
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    PostProcessSettings settings;
    pipeline.apply(source, scratch, {output.framebuffer(), 16, 16, false}, settings);
    check(pixel(2, 2)[0] < pixel(2, 14)[0], "postprocess must preserve vertical orientation");
    const auto original = pixel(7, 2);
    settings.enabled = true;
    settings.effect = PostProcessEffect::Grayscale;
    pipeline.apply(source, scratch, {output.framebuffer(), 16, 16, false}, settings);
    auto gray = pixel(2, 2);
    check(std::abs(int(gray[0]) - int(gray[1])) < 3 &&
              std::abs(int(gray[1]) - int(gray[2])) < 3, "grayscale channels must match");
    settings.effect = PostProcessEffect::Invert;
    pipeline.apply(source, scratch, {output.framebuffer(), 16, 16, false}, settings);
    check(pixel(2, 2)[0] > 215, "invert must operate on linear scene values");
    settings.effect = PostProcessEffect::Sharpen;
    pipeline.apply(source, scratch, {output.framebuffer(), 16, 16, false}, settings);
    check(pixel(7, 2)[1] < original[1], "sharpen must increase edge contrast");
    settings.effect = PostProcessEffect::Blur;
    settings.radius = 3;
    pipeline.apply(source, scratch, {output.framebuffer(), 16, 16, false}, settings);
    check(pixel(7, 2)[1] > original[1] + 5, "blur must spread colors across the edge");
    check(std::abs(int(pixel(2, 2)[0]) - int(original[0])) < 3,
          "two-pass blur must encode the display only once");
    source.resize(32, 24);
    check(source.width() == 32 && source.height() == 24 && source.complete(), "FBO must resize");
    source.resize(32, 24, GL_DEPTH32F_STENCIL8);
    output.resize(32, 24, GL_DEPTH32F_STENCIL8);
    source.bindForScene();
    RenderState::prepareClear();
    glClearDepth(.33);
    glClearStencil(5);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    settings.enabled = false;
    pipeline.apply(source, scratch, {output.framebuffer(), 32, 24, false}, settings);
    float depth = 0;
    unsigned char stencil = 0;
    glReadPixels(2, 2, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    glReadPixels(2, 2, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, &stencil);
    check(std::abs(depth - .33f) < .01f && stencil == 5,
          "postprocess must preserve D32F/S8 depth and stencil values");
    checkGl("postprocess effects and resize");
}
