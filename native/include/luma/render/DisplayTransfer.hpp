#pragma once
#include <luma/gfx/IResourceFactory.hpp>

namespace luma {
// Only used when the host has no sRGB-capable output attachment. Blending must
// finish in a linear intermediate before the final display encoding.
// User FBO output must be single-sample packed D24/S8 or D32F/S8.
// Default framebuffer precision metadata is adapted separately (see probe).
// The intermediate matches that format so final depth/stencil values survive.
class DisplayTransfer {
  public:
    explicit DisplayTransfer(IResourceFactory& factory) : factory_(&factory) {}
    ~DisplayTransfer();
    DisplayTransfer(const DisplayTransfer&) = delete;
    DisplayTransfer& operator=(const DisplayTransfer&) = delete;
    DisplayTransfer(DisplayTransfer&&) noexcept;
    DisplayTransfer& operator=(DisplayTransfer&&) noexcept;
    void initialize(const std::filesystem::path& shaderDirectory);
    unsigned prepare(int width, int height, unsigned outputFramebuffer = 0);
    void present(unsigned framebuffer, int width, int height);

  private:
    IResourceFactory* factory_;
    std::unique_ptr<IShader> shader_;
    unsigned framebuffer_ = 0, color_ = 0, depthStencil_ = 0, vao_ = 0;
    int width_ = 0, height_ = 0;
    unsigned depthStencilFormat_ = 0;
    void releaseAttachments() noexcept;
};
} // namespace luma
