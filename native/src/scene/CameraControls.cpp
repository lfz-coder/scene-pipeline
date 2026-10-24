#include <algorithm>
#include <cmath>
#include <glm/gtc/quaternion.hpp>
#include <luma/scene/CameraControls.hpp>

namespace luma {
void CameraControls::update(Camera& camera, const InputSnapshot& input, float dt) {
    if (!input.focused)
        return;

    auto eye = camera.eye();
    auto target = camera.target();
    const auto up = camera.up();
    auto forward = glm::normalize(target - eye);
    float distance = glm::distance(eye, target);
    const bool drag = mode_ == CameraMode::Orbit ? input.orbitDrag : input.flyDrag;
    const bool rotate = drag && (input.mouseX != 0 || input.mouseY != 0);
    bool changed = false;

    if (rotate) {
        // Turntable controls use the camera's supplied up reference, including
        // rolled and top-down cameras. No implicit world-Y reset is performed.
        if (input.mouseX != 0) {
            const auto yaw = glm::angleAxis(-static_cast<float>(input.mouseX) * 0.006f, up);
            forward = glm::normalize(yaw * forward);
        }
        if (input.mouseY != 0) {
            const auto right = glm::normalize(glm::cross(forward, up));
            const float pitch = std::asin(glm::clamp(glm::dot(forward, up), -1.0f, 1.0f));
            const float nextPitch =
                glm::clamp(pitch - static_cast<float>(input.mouseY) * 0.006f, -1.52f, 1.52f);
            forward = glm::normalize(glm::angleAxis(nextPitch - pitch, right) * forward);
        }
        changed = true;
    }

    if (mode_ == CameraMode::Orbit) {
        // Limits constrain a real dolly, not an idle update or a mode switch.
        if (input.scroll != 0) {
            distance = glm::clamp(distance * std::exp(-static_cast<float>(input.scroll) * 0.12f),
                                  0.5f, 60.0f);
            changed = true;
        }
        if (changed)
            eye = target - forward * distance;
    } else {
        const auto right = glm::normalize(glm::cross(forward, up));
        glm::vec3 movement(0);
        if (input.down(Key::W))
            movement += forward;
        if (input.down(Key::S))
            movement -= forward;
        if (input.down(Key::D))
            movement += right;
        if (input.down(Key::A))
            movement -= right;
        if (input.down(Key::E))
            movement += up;
        if (input.down(Key::Q))
            movement -= up;
        if (glm::length(movement) > 0 && dt > 0) {
            eye += glm::normalize(movement) * dt * (input.down(Key::Shift) ? 10.0f : 3.0f);
            changed = true;
        }
        if (changed)
            target = eye + forward * distance;
    }

    // Preserve the exact original pose (including roll and distance) when no
    // input changes it. Rebuilding lookAt every idle frame would cause drift.
    if (changed)
        camera.lookAt(eye, target, up);
}
} // namespace luma
