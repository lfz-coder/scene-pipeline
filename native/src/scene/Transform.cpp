#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <luma/scene/Transform.hpp>
#include <stdexcept>
namespace luma {
namespace {
bool finite(glm::vec3 v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
} // namespace
void Transform::setPosition(glm::vec3 p) {
    if (!finite(p))
        throw std::invalid_argument("position must be finite");
    position_ = p;
}
void Transform::setRotation(glm::quat q) {
    if (!std::isfinite(q.w) || !finite({q.x, q.y, q.z}))
        throw std::invalid_argument("rotation must be a finite nonzero quaternion");
    // Double precision avoids both overflow for large finite components and
    // underflow for tiny, valid nonzero quaternions.
    double length =
        std::sqrt(double(q.w) * q.w + double(q.x) * q.x + double(q.y) * q.y + double(q.z) * q.z);
    if (length == 0)
        throw std::invalid_argument("rotation must be a finite nonzero quaternion");
    rotation_ = {float(q.w / length), float(q.x / length), float(q.y / length),
                 float(q.z / length)};
}
void Transform::setScale(glm::vec3 s) {
    if (!finite(s) || s.x <= 0 || s.y <= 0 || s.z <= 0)
        throw std::invalid_argument("scale must be finite and positive");
    scale_ = s;
}
glm::mat4 Transform::matrix() const {
    return glm::translate(glm::mat4(1), position_) * glm::mat4_cast(rotation_) *
           glm::scale(glm::mat4(1), scale_);
}
} // namespace luma
