#pragma once
#include <string>
struct GLFWwindow;
namespace luma {
class GlfwRuntime {
  public:
    GlfwRuntime();
    ~GlfwRuntime();
    GlfwRuntime(const GlfwRuntime&) = delete;
    GlfwRuntime& operator=(const GlfwRuntime&) = delete;
};
class GlfwWindow {
  public:
    GlfwWindow(const std::string& title, int logicalWidth, int logicalHeight, bool hidden);
    ~GlfwWindow();
    GlfwWindow(const GlfwWindow&) = delete;
    GlfwWindow& operator=(const GlfwWindow&) = delete;
    GLFWwindow* native() const {
        return window_;
    }
    void framebufferSize(int& pixelWidth, int& pixelHeight) const;
    bool srgbAttachment() const;
    bool shouldClose() const;
    void poll();
    void swap();

  private:
    GLFWwindow* window_ = nullptr;
};
} // namespace luma
