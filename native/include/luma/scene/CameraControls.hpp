#pragma once
#include <luma/core/Input.hpp>
#include <luma/scene/Camera.hpp>
#include <luma/scene/Types.hpp>
namespace luma {
// Turntable yaw/pitch and Fly movement honor Camera::up(). Idle input and
// mode changes preserve the complete pose; Orbit limits apply only to dolly.
class CameraControls {
  public:
    void setMode(CameraMode mode) {
        mode_ = mode;
    }
    CameraMode mode() const {
        return mode_;
    }
    void update(Camera& camera, const InputSnapshot& input, float deltaSeconds);

  private:
    CameraMode mode_ = CameraMode::Orbit;
};
} // namespace luma
