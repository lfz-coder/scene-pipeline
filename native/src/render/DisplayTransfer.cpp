#include <luma/gfx/RenderState.hpp>
#include <luma/render/DisplayTransfer.hpp>
#include <luma/render/RenderTarget.hpp>
#include <stdexcept>
#include <string>
#include <utility>

namespace luma {
void DisplayTransfer::releaseAttachments() noexcept {
    if (depthStencil_)
        glDeleteRenderbuffers(1, &depthStencil_);
    if (color_)
        glDeleteTextures(1, &color_);
    if (framebuffer_)
        glDeleteFramebuffers(1, &framebuffer_);
    depthStencil_ = color_ = framebuffer_ = 0;
    width_ = height_ = 0;
    depthStencilFormat_ = 0;
}
DisplayTransfer::~DisplayTransfer() {
    releaseAttachments();
    if (vao_)
        glDeleteVertexArrays(1, &vao_);
}
DisplayTransfer::DisplayTransfer(DisplayTransfer&& o) noexcept
    : factory_(o.factory_), shader_(std::move(o.shader_)),
      framebuffer_(std::exchange(o.framebuffer_, 0)), color_(std::exchange(o.color_, 0)),
      depthStencil_(std::exchange(o.depthStencil_, 0)), vao_(std::exchange(o.vao_, 0)),
      width_(std::exchange(o.width_, 0)), height_(std::exchange(o.height_, 0)),
      depthStencilFormat_(std::exchange(o.depthStencilFormat_, 0)) {}
DisplayTransfer& DisplayTransfer::operator=(DisplayTransfer&& o) noexcept {
    if (this != &o) {
        releaseAttachments();
        if (vao_)
            glDeleteVertexArrays(1, &vao_);
        factory_ = o.factory_;
        shader_ = std::move(o.shader_);
        framebuffer_ = std::exchange(o.framebuffer_, 0);
        color_ = std::exchange(o.color_, 0);
        depthStencil_ = std::exchange(o.depthStencil_, 0);
        vao_ = std::exchange(o.vao_, 0);
        width_ = std::exchange(o.width_, 0);
        height_ = std::exchange(o.height_, 0);
        depthStencilFormat_ = std::exchange(o.depthStencilFormat_, 0);
    }
    return *this;
}
void DisplayTransfer::initialize(const std::filesystem::path& dir) {
    auto shader = factory_->createShader(dir / "display.vert", dir / "display.frag");
    if (!shader)
        throw std::runtime_error("resource factory returned null display shader");
    requireContext();
    shader_ = std::move(shader);
    if (!vao_)
        glGenVertexArrays(1, &vao_);
}
unsigned DisplayTransfer::prepare(int width, int height, unsigned outputFramebuffer) {
    requireContext();
    if (width <= 0 || height <= 0)
        throw std::invalid_argument("linear display dimensions must be positive");
    const GLenum format = destinationDepthStencilFormat(outputFramebuffer);
    if (width_ == width && height_ == height && depthStencilFormat_ == format)
        return framebuffer_;
    releaseAttachments();
    glGenFramebuffers(1, &framebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glGenTextures(1, &color_);
    glBindTexture(GL_TEXTURE_2D, color_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_, 0);
    glGenRenderbuffers(1, &depthStencil_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencil_);
    glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              depthStencil_);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("linear display framebuffer incomplete");
    checkGl("linear display allocation");
    width_ = width;
    height_ = height;
    depthStencilFormat_ = format;
    return framebuffer_;
}
void DisplayTransfer::present(unsigned output, int width, int height) {
    if (destinationDepthStencilFormat(output) != depthStencilFormat_)
        throw std::invalid_argument(
            "DisplayTarget depth/stencil changed after DisplayTransfer::prepare");
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, output);
    // Preserve the public output's depth/stencil semantics as well as its color.
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                      GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, output);
    glDisable(GL_FRAMEBUFFER_SRGB);
    RenderState state;
    state.width = width;
    state.height = height;
    state.depthTest = false;
    state.depthWrite = false;
    state.cull = false;
    state.apply();
    shader_->use();
    shader_->set("uLinearScene", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, color_);
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    RenderState baseline;
    baseline.width = width;
    baseline.height = height;
    baseline.apply();
    checkGl("final display encoding");
}
} // namespace luma
