#pragma once
#include <glm/glm.hpp>
#include <luma/scene/Types.hpp>
#include <optional>
namespace luma {
// Unlit material; RGB is linear, alpha is coverage/opacity.
// MaterialLibrary, EditorController and Renderer validate editable CPU values.
struct MaterialDesc {
    glm::vec4 baseColor{1};
    glm::vec3 specularColor{0.5f};
    float shininess = 32.0f;
    std::optional<TextureId> baseColorTexture;
    std::optional<TextureId> secondaryColorTexture;
    std::optional<TextureId> specularTexture;
    float textureMix = 0.5f;
    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 0.5f;
    bool doubleSided = false;
    static MaterialDesc unlit(glm::vec4 color) {
        MaterialDesc m;
        m.baseColor = color;
        return m;
    }
    void validate() const;
};
} // namespace luma
