#include <cmath>
#include <iostream>
#include <limits>
#include <luma/editor/EditorController.hpp>
#include <stdexcept>
using namespace luma;
void check(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
template <class F> void rejects(F f) {
    bool failed = false;
    try {
        f();
    } catch (const std::exception&) {
        failed = true;
    }
    check(failed, "invalid input accepted");
}
int main() {
    Scene scene;
    MaterialLibrary materials;
    Camera camera;
    CameraControls controls;
    auto material = materials.createMaterial(MaterialDesc::unlit({1, 0, 0, 1}));
    auto a = scene.create("a", {1}, material), b = scene.create("b", {1}, material);
    EditorModel model(scene, materials, camera, controls);
    EditorController edit(model);
    edit.select(a);
    auto pointId = edit.addPointLight({});
    check(pointId.value == 1 && model.lights().entries().size() == 1, "controller adds light");
    auto point = std::get<PointLight>(model.lights().entry(pointId).value);
    point.intensity = 2;
    edit.updateLight(pointId, point);
    check(std::get<PointLight>(model.lights().entry(pointId).value).intensity == 2,
          "controller updates light");
    auto badPoint = point;
    badPoint.constant = 0;
    rejects([&] { edit.updateLight(pointId, badPoint); });
    check(std::get<PointLight>(model.lights().entry(pointId).value).constant == point.constant,
          "invalid light update partially committed");
    auto lighting = model.lighting();
    lighting.model = ShadingModel::BlinnPhong;
    edit.setLightingSettings(lighting);
    check(model.lighting().model == ShadingModel::BlinnPhong, "lighting settings");
    auto post = model.postProcess();
    post.enabled = true;
    post.effect = PostProcessEffect::Grayscale;
    edit.setPostProcessSettings(post);
    check(model.postProcess().effect == PostProcessEffect::Grayscale, "post process settings");
    edit.setTransform({1, 2, 3}, {1, 0, 0, 0}, {2, 2, 2});
    rejects([&] { edit.setTransform({9, 9, 9}, {1, 0, 0, 0}, {0, 1, 1}); });
    check(scene.transform(a).position() == glm::vec3(1, 2, 3) &&
              scene.transform(a).scale() == glm::vec3(2) &&
              scene.transform(a).rotation() == glm::quat(1, 0, 0, 0),
          "transform partially committed");
    rejects([&] { edit.setTransform({9, 9, 9}, {0, 0, 0, 0}, {1, 1, 1}); });
    check(scene.transform(a).position() == glm::vec3(1, 2, 3),
          "rotation failure partially committed");
    check(scene.transform(b).position() == glm::vec3(0), "transform shared");
    auto color = materials.material(material);
    color.baseColor = {0, 1, 0, 1};
    edit.setMaterial(color);
    check(materials.material(scene.renderItems()[1].material).baseColor.g == 1,
          "shared material lost");
    auto clone = edit.cloneSelectedMaterial();
    color.baseColor = {0, 0, 1, 1};
    color.baseColorTexture = TextureId{1};
    color.secondaryColorTexture = TextureId{2};
    color.textureMix = 0.25f;
    edit.setMaterial(color);
    check(materials.material(material).baseColor.g == 1 &&
              materials.material(clone).baseColor.b == 1,
          "clone not independent");
    check(materials.material(clone).textureMix == 0.25f &&
              materials.material(clone).secondaryColorTexture->value == 2 &&
              !materials.material(material).secondaryColorTexture,
          "controller must commit dual-texture edit only to selected independent material");
    auto invalidMix = color;
    invalidMix.textureMix = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { edit.setMaterial(invalidMix); });
    check(materials.material(clone).textureMix == 0.25f,
          "invalid mix must not partially commit a material edit");
    auto invalid = color;
    invalid.alphaMode = static_cast<AlphaMode>(99);
    rejects([&] { edit.setMaterial(invalid); });
    check(materials.material(clone).alphaMode == color.alphaMode, "invalid alpha mode committed");
    invalid = color;
    invalid.alphaCutoff = 2;
    rejects([&] { edit.setMaterial(invalid); });
    check(materials.material(clone).alphaCutoff == color.alphaCutoff,
          "material partially committed");
    auto oldEye = camera.eye(), oldTarget = camera.target(), oldUp = camera.up();
    edit.setCameraMode(CameraMode::Fly);
    check(model.controls().mode() == CameraMode::Fly, "camera mode");
    check(camera.eye() == oldEye && camera.target() == oldTarget && camera.up() == oldUp,
          "mode changed pose");
    auto eye = camera.eye();
    rejects([&] { edit.setCameraPose({0, 0, 0}, {0, 0, 0}); });
    check(camera.eye() == eye, "invalid camera mutated");
    edit.setCameraPose({2, 3, 4}, {0, 0, 0});
    check(camera.eye() == glm::vec3(2, 3, 4), "camera pose");
    RenderOptions options = model.options();
    options.depth = false;
    options.cull = false;
    options.blend = false;
    options.outline = false;
    options.selected = b;
    edit.setRenderOptions(options);
    edit.setStyle(StyleKind::Wireframe);
    check(!model.options().depth && !model.options().cull && !model.options().blend &&
              !model.options().outline && model.style() == StyleKind::Wireframe,
          "view settings");
    check(model.selected().value == a.value, "render flags bypassed selection");
    options.background.x = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { edit.setRenderOptions(options); });
    check(std::isfinite(model.options().background.x), "settings partially committed");
    rejects([&] { edit.setCameraMode(static_cast<CameraMode>(99)); });
    rejects([&] { edit.setStyle(static_cast<StyleKind>(99)); });
    rejects([&] { edit.select({999}); });
    check(model.selected().value == a.value, "invalid selection mutated");
    scene.erase(a);
    edit.reconcileSelection();
    check(!model.selected().value, "stale selection");
    rejects([&] { edit.setMaterial(color); });
    edit.select(b);
    edit.clearSelection();
    check(!model.selected().value, "clear selection");
    edit.select(b);
    scene.clear();
    edit.reconcileSelection();
    check(!model.selected().value, "clear retained stale selection");
    rejects([&] { edit.cloneSelectedMaterial(); });
    std::cout << "controller behaviors passed\n";
}
