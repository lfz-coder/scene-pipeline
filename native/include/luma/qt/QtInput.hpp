#pragma once
#include <luma/core/Input.hpp>
#include <QPointF>
namespace luma {
// Qt event policy only. The reusable camera consumes InputSnapshot.
class QtInput {
  public:
    void key(int qtKey, bool down);
    void pointer(QPointF position, bool orbit, bool fly);
    void wheel(double steps) { state_.scroll += steps; }
    void reset();
    InputSnapshot take();
  private:
    InputSnapshot state_{};
    QPointF last_;
    bool pointerKnown_ = false;
};
}
