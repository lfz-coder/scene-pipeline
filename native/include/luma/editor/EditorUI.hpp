#pragma once
#include <luma/editor/EditorController.hpp>
#include <string>
struct GLFWwindow;
namespace luma {
class EditorUI {
  public:
    explicit EditorUI(GLFWwindow*);
    ~EditorUI();
    EditorUI(const EditorUI&) = delete;
    EditorUI& operator=(const EditorUI&) = delete;
    void beginFrame();
    void endWithoutDraw();
    bool captureMouse() const;
    bool captureKeyboard() const;
    std::string inspect(const EditorModel& model, EditorController& controller,
                        double frameSeconds);
    void draw();
};
} // namespace luma
