#pragma once
#include <glad/gl.h>

namespace luma {

class PostProcessTarget {
  public:
    PostProcessTarget() = default;
    ~PostProcessTarget();
    PostProcessTarget(const PostProcessTarget&) = delete;
    PostProcessTarget& operator=(const PostProcessTarget&) = delete;
    PostProcessTarget(PostProcessTarget&& other) noexcept;
    PostProcessTarget& operator=(PostProcessTarget&& other) noexcept;
    void resize(int width, int height, unsigned depthStencilFormat = GL_DEPTH24_STENCIL8);
    void bindForScene() const;
    bool complete() const;
    unsigned framebuffer() const {
        return framebuffer_;
    }
    unsigned texture() const {
        return texture_;
    }
    int width() const {
        return width_;
    }
    int height() const {
        return height_;
    }
    unsigned depthStencilFormat() const { return depthStencilFormat_; }

  private:
    GLuint framebuffer_ = 0, texture_ = 0, depthStencil_ = 0;
    int width_ = 0, height_ = 0;
    unsigned depthStencilFormat_ = GL_DEPTH24_STENCIL8;
    void release() noexcept;
};
} // namespace luma
