#include <luma/editor/EditorController.hpp>
#include <luma/render/Renderer.hpp>
#include <iostream>
#include <stdexcept>

namespace {
// Reusable CPU recipe: no window, QWidget, shader compilation or GL calls.
void configureStudio(luma::EditorController& controller) {
    luma::LightingSettings settings;
    settings.model = luma::ShadingModel::BlinnPhong;
    settings.ambientStrength = .08f;
    controller.setLightingSettings(settings);
    controller.clearLights();
    controller.addDirectionalLight({{-.4f, -1, -.3f}, {1, .9f, .8f}, 1.2f, true});
    controller.addPointLight({{-2, 2, 2}, {1, .2f, .1f}, 2, 1, .09f, .032f, true});
    controller.addSpotLight({{2, 3, 3}, {-.2f, -1, -1}, {.2f, .5f, 1}, 3,
                              1, .09f, .032f, 10, 24, true});
    luma::PostProcessSettings post;
    post.enabled = true;
    post.effect = luma::PostProcessEffect::Grayscale;
    post.intensity = .35f;
    controller.setPostProcessSettings(post);
}
} // namespace

// A Qt or GLFW host calls this after it has initialized its Renderer.
// Setting CPU values needs no context; render() still requires a current context.
void synchronizeStudio(luma::Renderer& renderer, const luma::EditorModel& model) {
    renderer.setLighting(model.lighting(), model.lights());
    renderer.setPostProcess(model.postProcess());
}

int main() {
    luma::Scene scene;
    luma::MaterialLibrary materials;
    luma::Camera camera;
    luma::CameraControls controls;
    luma::EditorModel model(scene, materials, camera, controls);
    luma::EditorController controller(model);
    auto material = luma::MaterialDesc::unlit({.2f, .4f, .8f, 1});
    material.specularColor = {.9f, .9f, 1};
    material.shininess = 96;
    auto materialId = materials.createMaterial(material);
    auto entity = scene.create("无窗口光照配方", {1}, materialId);
    controller.select(entity);
    configureStudio(controller);
    if (model.lights().entries().size() != 3 || model.lighting().ambientStrength != .08f)
        throw std::runtime_error("lighting recipe did not reach the CPU model");
    std::cout << "PASS lighting example: 3 lights, shininess 96, grayscale intensity 0.35; no window\n";
}
