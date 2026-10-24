#include <glm/gtc/matrix_inverse.hpp>
#include <cmath>
#include <luma/gfx/RenderState.hpp>
#include <luma/lighting/LightRig.hpp>
#include <luma/render/LightingPipeline.hpp>
#include <luma/scene/AssetStore.hpp>
#include <luma/scene/Camera.hpp>
#include <luma/scene/Scene.hpp>
#include <stdexcept>
#include <string>

namespace luma {
void LightingPipeline::initialize(IResourceFactory& factory,
                                  const std::filesystem::path& shaderDirectory) {
    auto shader = factory.createShader(shaderDirectory / "lighting.vert",
                                       shaderDirectory / "lighting.frag");
    if (!shader)
        throw std::runtime_error("resource factory returned null lighting shader");
    factory_ = &factory;
    shader_ = std::move(shader);
}

void LightingPipeline::uploadLights(const LightRig& rig, int budget) {
    shader_->set("uDirectional.enabled", 0);
    int points = 0, spots = 0;
    for (const auto& entry : rig.entries()) {
        if (budget-- <= 0) break;
        if (entry.kind == LightKind::Directional) {
            const auto& light = std::get<DirectionalLight>(entry.value);
            shader_->set("uDirectional.direction", light.direction);
            shader_->set("uDirectional.color", light.color);
            shader_->set("uDirectional.intensity", light.intensity);
            shader_->set("uDirectional.enabled", light.enabled ? 1 : 0);
        } else if (entry.kind == LightKind::Point && points < 8) {
            const auto& light = std::get<PointLight>(entry.value);
            const auto prefix = "uPoints[" + std::to_string(points++) + "]";
            shader_->set(prefix + ".position", light.position);
            shader_->set(prefix + ".color", light.color);
            shader_->set(prefix + ".intensity", light.intensity);
            shader_->set(prefix + ".attenuation",
                         glm::vec3{light.constant, light.linear, light.quadratic});
            shader_->set(prefix + ".enabled", light.enabled ? 1 : 0);
        } else if (entry.kind == LightKind::Spot && spots < 4) {
            const auto& light = std::get<SpotLight>(entry.value);
            const auto prefix = "uSpots[" + std::to_string(spots++) + "]";
            shader_->set(prefix + ".position", light.position);
            shader_->set(prefix + ".direction", light.direction);
            shader_->set(prefix + ".color", light.color);
            shader_->set(prefix + ".intensity", light.intensity);
            shader_->set(prefix + ".attenuation",
                         glm::vec3{light.constant, light.linear, light.quadratic});
            constexpr float radians = 0.0174532925199433f;
            shader_->set(prefix + ".cone",
                         glm::vec3{std::cos(light.innerCutoffDegrees * radians),
                                   std::cos(light.outerCutoffDegrees * radians), 0});
            shader_->set(prefix + ".enabled", light.enabled ? 1 : 0);
        }
    }
    shader_->set("uPointCount", points);
    shader_->set("uSpotCount", spots);
}

void LightingPipeline::draw(const RenderItem& item, const Scene& scene, const AssetStore& assets,
                            const Camera& camera, const LightRig& rig,
                            const LightingSettings& settings, bool manualEncoding) {
    requireContext();
    if (!shader_ || !factory_)
        throw std::logic_error("LightingPipeline not initialized");
    settings.validate();
    rig.validate();
    shader_->use();
    const auto& material = assets.material(item.material);
    material.validate();
    const auto model = scene.transform(item.entity).matrix();
    shader_->set("uModel", model);
    shader_->set("uView", camera.view());
    shader_->set("uProjection", camera.projection());
    shader_->set("uNormalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
    shader_->set("uViewPosition", camera.eye());
    shader_->set("uBaseColor", material.baseColor);
    shader_->set("uSpecularColor", material.specularColor);
    shader_->set("uShininess", material.shininess);
    shader_->set("uAmbientColor", settings.ambientColor);
    shader_->set("uAmbientStrength", settings.ambientStrength);
    shader_->set("uManualEncoding", manualEncoding ? 1 : 0);
    shader_->set("uVisualizeNormals", settings.visualizeNormals ? 1 : 0);
    shader_->set("uUseTexture", material.baseColorTexture ? 1 : 0);
    shader_->set("uUseTexture2", material.secondaryColorTexture ? 1 : 0);
    shader_->set("uTextureMix", material.textureMix);
    shader_->set("uAlphaMode", static_cast<int>(material.alphaMode));
    shader_->set("uAlphaCutoff", material.alphaCutoff);
    shader_->set("uUseSpecularTexture", material.specularTexture ? 1 : 0);
    shader_->set("uTexture", 0);
    shader_->set("uTexture2", 1);
    shader_->set("uSpecularTexture", 2);
    if (material.baseColorTexture)
        assets.texture(*material.baseColorTexture).bind(0);
    if (material.secondaryColorTexture)
        assets.texture(*material.secondaryColorTexture).bind(1);
    if (material.specularTexture)
        assets.texture(*material.specularTexture).bind(2);
    uploadLights(rig, settings.maxLights);
    assets.mesh(item.mesh).draw();
}
} // namespace luma
