#pragma once
#include <array>
namespace luma {
enum class Key : unsigned { W, A, S, D, Q, E, Shift, Count };
struct InputSnapshot {
    std::array<bool, 7> held{}, pressed{}, released{};
    bool focused = true, orbitDrag = false, flyDrag = false;
    double mouseX = 0, mouseY = 0, scroll = 0;
    bool down(Key key) const {
        return held[static_cast<unsigned>(key)];
    }
};
class InputRouter {
  public:
    InputSnapshot poll(void* nativeWindow, bool captureMouse, bool captureKeyboard);

  private:
    InputSnapshot previous_{};
    double x_ = 0, y_ = 0;
    bool initialized_ = false;
};
} // namespace luma
