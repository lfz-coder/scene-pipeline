#pragma once
#include <glm/glm.hpp>

namespace luma {

enum class ShadingModel { Unlit, BlinnPhong };

struct LightingSettings {
    ShadingModel model = ShadingModel::Unlit;
    glm::vec3 ambientColor{1.0f};
    float ambientStrength = 0.15f;
    bool visualizeNormals = false;
    int maxLights = 13;
    void validate() const;
};

} // namespace luma
