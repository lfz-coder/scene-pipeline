#include <luma/qt/QtInput.hpp>
#include <Qt>
namespace luma {
void QtInput::key(int code, bool down) {
    int index = -1;
    switch(code) {
        case Qt::Key_W: index=0; break; case Qt::Key_A: index=1; break;
        case Qt::Key_S: index=2; break; case Qt::Key_D: index=3; break;
        case Qt::Key_Q: index=4; break; case Qt::Key_E: index=5; break;
        case Qt::Key_Shift: index=6; break; default: return;
    }
    if (state_.held[index] != down) {
        state_.pressed[index] = down; state_.released[index] = !down;
    }
    state_.held[index] = down;
}
void QtInput::pointer(QPointF pos, bool orbit, bool fly) {
    if (pointerKnown_) {
        state_.mouseX += pos.x() - last_.x();
        state_.mouseY += pos.y() - last_.y();
    }
    last_ = pos; pointerKnown_ = true;
    state_.orbitDrag = orbit; state_.flyDrag = fly;
}
void QtInput::reset() { state_={}; pointerKnown_=false; }
InputSnapshot QtInput::take() {
    auto result = state_;
    state_.mouseX=state_.mouseY=state_.scroll=0;
    state_.pressed.fill(false); state_.released.fill(false);
    return result;
}
}
