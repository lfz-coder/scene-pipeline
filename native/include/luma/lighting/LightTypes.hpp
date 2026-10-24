#pragma once
#include <cstdint>
#include <glm/glm.hpp>

namespace luma {

struct LightId {
    std::uint32_t value = 0;
    friend bool operator==(LightId a, LightId b) {
        return a.value == b.value;
    }
};

struct DirectionalLight {
    glm::vec3 direction{-0.4f, -1.0f, -0.3f};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    bool enabled = true;
    void validate() const;
};

struct PointLight {
    glm::vec3 position{1.5f, 2.4f, 2.0f};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    bool enabled = true;
    void validate() const;
};

struct SpotLight {
    glm::vec3 position{0.0f, 3.0f, 3.0f};
    glm::vec3 direction{0.0f, -0.7f, -1.0f};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    float innerCutoffDegrees = 12.5f;
    float outerCutoffDegrees = 17.5f;
    bool enabled = true;
    void validate() const;
};

} // namespace luma
