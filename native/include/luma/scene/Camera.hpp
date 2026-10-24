#pragma once
#include <glm/glm.hpp>
namespace luma {
// CPU camera data only: input policy belongs to CameraControls.
// Invalid parameters throw before changing the previous valid camera.
class Camera {
  public:
    void lookAt(glm::vec3 eye, glm::vec3 target, glm::vec3 up = {0, 1, 0});
    void setPerspective(float verticalFovDegrees, float nearPlane, float farPlane);
    void setAspect(float framebufferAspect);
    glm::mat4 view() const;
    glm::mat4 projection() const;
    glm::vec3 eye() const {
        return eye_;
    }
    glm::vec3 up() const {
        return up_;
    }
    glm::vec3 target() const {
        return target_;
    }

  private:
    glm::vec3 eye_{4, 3, 6}, target_{0}, up_{0, 1, 0};
    float fov_ = 45, near_ = 0.1f, far_ = 100, aspect_ = 1;
};
} // namespace luma
