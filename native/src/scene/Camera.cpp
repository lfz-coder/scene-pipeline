#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <luma/scene/Camera.hpp>
#include <stdexcept>
namespace luma {
namespace {
bool finite(glm::vec3 v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
void validateProjection(float fov, float aspect, float nearPlane, float farPlane) {
    auto matrix = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            if (!std::isfinite(matrix[column][row]))
                throw std::invalid_argument("perspective parameters exceed finite matrix range");
}
} // namespace
void Camera::lookAt(glm::vec3 eye, glm::vec3 target, glm::vec3 up) {
    auto direction = target - eye;
    float distance = glm::length(direction), upLength = glm::length(up);
    if (!finite(eye) || !finite(target) || !finite(up) || !finite(direction) ||
        !std::isfinite(distance) || !std::isfinite(upLength) || distance < 1e-5f ||
        upLength < 1e-5f)
        throw std::invalid_argument(
            "Camera lookAt requires finite, distinct eye/target and valid up");
    // Normalize before crossing to prevent finite, large coordinates overflowing.
    if (glm::length(glm::cross(direction / distance, up / upLength)) < 1e-5f)
        throw std::invalid_argument("Camera lookAt up must not parallel the view direction");
    eye_ = eye;
    target_ = target;
    up_ = up / upLength;
}
void Camera::setPerspective(float f, float n, float z) {
    if (!std::isfinite(f) || !std::isfinite(n) || !std::isfinite(z) || f <= 0 || f >= 180 ||
        n <= 0 || z <= n)
        throw std::invalid_argument("invalid perspective FOV/near/far");
    validateProjection(f, aspect_, n, z);
    fov_ = f;
    near_ = n;
    far_ = z;
}
void Camera::setAspect(float a) {
    if (!std::isfinite(a) || a <= 0)
        throw std::invalid_argument("aspect must be finite and positive");
    validateProjection(fov_, a, near_, far_);
    aspect_ = a;
}
glm::mat4 Camera::view() const {
    return glm::lookAt(eye_, target_, up_);
}
glm::mat4 Camera::projection() const {
    return glm::perspective(glm::radians(fov_), aspect_, near_, far_);
}
} // namespace luma
