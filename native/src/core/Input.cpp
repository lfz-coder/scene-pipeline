#include <luma/core/Input.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
namespace luma {
// Scroll is installed through window user data separately by Application; ImGui
// owns callback chaining. Polling key state avoids stealing its key callbacks.
InputSnapshot InputRouter::poll(void* native, bool mouseCapture, bool keyCapture) {
    auto* w = static_cast<GLFWwindow*>(native);
    InputSnapshot s;
    s.focused = glfwGetWindowAttrib(w, GLFW_FOCUSED) != 0;
    double x, y;
    glfwGetCursorPos(w, &x, &y);
    if (!s.focused) {
        previous_ = {};
        initialized_ = false;
        return s;
    }
    if (initialized_ && !mouseCapture) {
        s.mouseX = x - x_;
        s.mouseY = y - y_;
    }
    x_ = x;
    y_ = y;
    initialized_ = true;
    const int keys[] = {GLFW_KEY_W, GLFW_KEY_A, GLFW_KEY_S,         GLFW_KEY_D,
                        GLFW_KEY_Q, GLFW_KEY_E, GLFW_KEY_LEFT_SHIFT};
    for (unsigned i = 0; i < 7; ++i) {
        s.held[i] = !keyCapture && glfwGetKey(w, keys[i]) == GLFW_PRESS;
        s.pressed[i] = s.held[i] && !previous_.held[i];
        s.released[i] = !s.held[i] && previous_.held[i];
    }
    s.orbitDrag = !mouseCapture && glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    s.flyDrag = !mouseCapture && glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    previous_ = s;
    return s;
}
} // namespace luma
