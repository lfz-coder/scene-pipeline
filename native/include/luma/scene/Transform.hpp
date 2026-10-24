#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
namespace luma {
// CPU-only TRS for column vectors: local scale, quaternion rotation, translation.
// Positive scales preserve winding and avoid singular transforms.
class Transform {
  public:
    void setPosition(glm::vec3 position);
    void setRotation(glm::quat rotation);
    void setScale(glm::vec3 positiveScale);
    glm::vec3 position() const {
        return position_;
    }
    glm::quat rotation() const {
        return rotation_;
    }
    glm::vec3 scale() const {
        return scale_;
    }
    glm::mat4 matrix() const;

  private:
    glm::vec3 position_{0}, scale_{1};
    glm::quat rotation_{1, 0, 0, 0};
};
} // namespace luma
