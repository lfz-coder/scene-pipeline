#include <luma/render/PostProcessTarget.hpp>
#include <luma/gfx/RenderState.hpp>
#include <stdexcept>
#include <utility>

namespace luma {
void PostProcessTarget::release() noexcept {
    if (depthStencil_)
        glDeleteRenderbuffers(1, &depthStencil_);
    if (texture_)
        glDeleteTextures(1, &texture_);
    if (framebuffer_)
        glDeleteFramebuffers(1, &framebuffer_);
    framebuffer_ = texture_ = depthStencil_ = 0;
    width_ = height_ = 0;
}

PostProcessTarget::~PostProcessTarget() {
    if (glGetString && glGetString(GL_VERSION))
        release();
}
PostProcessTarget::PostProcessTarget(PostProcessTarget&& other) noexcept
    : framebuffer_(std::exchange(other.framebuffer_, 0)),
      texture_(std::exchange(other.texture_, 0)),
      depthStencil_(std::exchange(other.depthStencil_, 0)),
      width_(std::exchange(other.width_, 0)), height_(std::exchange(other.height_, 0)),
      depthStencilFormat_(other.depthStencilFormat_) {}
PostProcessTarget& PostProcessTarget::operator=(PostProcessTarget&& other) noexcept {
    if (this != &other) {
        release();
        framebuffer_ = std::exchange(other.framebuffer_, 0);
        texture_ = std::exchange(other.texture_, 0);
        depthStencil_ = std::exchange(other.depthStencil_, 0);
        width_ = std::exchange(other.width_, 0);
        height_ = std::exchange(other.height_, 0);
        depthStencilFormat_ = other.depthStencilFormat_;
    }
    return *this;
}

void PostProcessTarget::resize(int width, int height, unsigned format) {
    if (width <= 0 || height <= 0 || width > 16384 || height > 16384)
        throw std::invalid_argument("post-process target dimensions are invalid");
    if (format != GL_DEPTH24_STENCIL8 && format != GL_DEPTH32F_STENCIL8)
        throw std::invalid_argument("post-process requires packed depth/stencil");
    requireContext();
    if (width == width_ && height == height_ && format == depthStencilFormat_ && complete())
        return;
    GLuint nextFramebuffer = 0, nextTexture = 0, nextDepth = 0;
    glGenFramebuffers(1, &nextFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, nextFramebuffer);
    glGenTextures(1, &nextTexture);
    glBindTexture(GL_TEXTURE_2D, nextTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, nextTexture, 0);
    glGenRenderbuffers(1, &nextDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, nextDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, format, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              nextDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glDeleteRenderbuffers(1, &nextDepth);
        glDeleteTextures(1, &nextTexture);
        glDeleteFramebuffers(1, &nextFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        throw std::runtime_error("post-process framebuffer is incomplete");
    }
    release();
    framebuffer_ = nextFramebuffer;
    texture_ = nextTexture;
    depthStencil_ = nextDepth;
    width_ = width;
    height_ = height;
    depthStencilFormat_ = format;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
}

void PostProcessTarget::bindForScene() const {
    if (!framebuffer_)
        throw std::logic_error("post-process target is not allocated");
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glViewport(0, 0, width_, height_);
}
bool PostProcessTarget::complete() const {
    if (!framebuffer_)
        return false;
    GLint previous = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    const bool result = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previous));
    return result;
}
} // namespace luma
