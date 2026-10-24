#include <luma/core/GlfwWindow.hpp>
#include <luma/gfx/RenderState.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
namespace luma {
namespace {
bool runtimeActive = false;
void error(int code, const char* text) {
    std::cerr << "GLFW " << code << ": " << text << '\n';
}
} // namespace
GlfwRuntime::GlfwRuntime() {
    if (runtimeActive)
        throw std::logic_error("only one GlfwRuntime supported");
    glfwSetErrorCallback(error);
    if (!glfwInit())
        throw std::runtime_error("glfwInit failed");
    runtimeActive = true;
}
GlfwRuntime::~GlfwRuntime() {
    glfwTerminate();
    runtimeActive = false;
}
GlfwWindow::GlfwWindow(const std::string& title, int w, int h, bool hidden) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SRGB_CAPABLE, GL_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    glfwWindowHint(GLFW_STENCIL_BITS, 8);
    glfwWindowHint(GLFW_VISIBLE, hidden ? GL_FALSE : GL_TRUE);
    window_ = glfwCreateWindow(w, h, title.c_str(), nullptr, nullptr);
    if (!window_)
        throw std::runtime_error("glfwCreateWindow failed (OpenGL 3.3 Core)");
    try {
        glfwMakeContextCurrent(window_);
        if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)))
            throw std::runtime_error("GLAD2 load failed");
        GLint depth, stencil;
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH,
                                              GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &depth);
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL,
                                              GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &stencil);
        checkGl("default depth/stencil query");
        if (depth < 16 || stencil < 8)
            throw std::runtime_error("default framebuffer requires depth>=16 and stencil>=8");
        glfwSwapInterval(hidden ? 0 : 1);
        std::cout << "OpenGL " << glGetString(GL_VERSION) << "; depth=" << depth
                  << " stencil=" << stencil
                  << "; output=" << (srgbAttachment() ? "sRGB hardware" : "manual sRGB") << '\n';
    } catch (...) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
        throw;
    }
}
GlfwWindow::~GlfwWindow() {
    if (window_)
        glfwDestroyWindow(window_);
}
void GlfwWindow::framebufferSize(int& w, int& h) const {
    glfwGetFramebufferSize(window_, &w, &h);
}
bool GlfwWindow::srgbAttachment() const {
    GLint encoding;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_BACK_LEFT,
                                          GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &encoding);
    checkGl("framebuffer encoding query");
    return encoding == GL_SRGB;
}
bool GlfwWindow::shouldClose() const {
    return glfwWindowShouldClose(window_);
}
void GlfwWindow::poll() {
    glfwPollEvents();
}
void GlfwWindow::swap() {
    glfwSwapBuffers(window_);
}
} // namespace luma
