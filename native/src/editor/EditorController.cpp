#include <cmath>
#include <luma/editor/EditorController.hpp>
#include <stdexcept>
#include <utility>
namespace luma {
MaterialId EditorModel::selectedMaterial() const {
    for (const auto& item : scene_.renderItems())
        if (item.entity.value == selected().value)
            return item.material;
    throw std::out_of_range("editor selection is absent");
}
void EditorController::select(EntityId entity) {
    if (!model_.scene_.contains(entity))
        throw std::out_of_range("cannot select absent entity");
    model_.options_.selected = entity;
}
void EditorController::clearSelection() {
    model_.options_.selected = {};
}
void EditorController::reconcileSelection() {
    if (!model_.scene_.contains(model_.selected()))
        clearSelection();
}
void EditorController::setTransform(glm::vec3 p, glm::quat r, glm::vec3 s) {
    auto candidate = model_.scene_.transform(model_.selected());
    candidate.setPosition(p);
    candidate.setRotation(r);
    candidate.setScale(s);
    model_.scene_.transform(model_.selected()) = candidate;
}
void EditorController::setMaterial(MaterialDesc material) {
    material.validate();
    model_.materials_.material(model_.selectedMaterial()) = material;
}
MaterialId EditorController::cloneSelectedMaterial() {
    auto source = model_.selectedMaterial();
    auto id = model_.materials_.cloneMaterial(source);
    model_.scene_.setMaterial(model_.selected(), id);
    return id;
}
void EditorController::setCameraMode(CameraMode mode) {
    if (mode != CameraMode::Orbit && mode != CameraMode::Fly)
        throw std::invalid_argument("invalid camera mode");
    model_.controls_.setMode(mode);
}
void EditorController::assignMaterial(MaterialId material) {
    model_.materials_.material(material).validate();
    model_.scene_.setMaterial(model_.selected(), material);
}
void EditorController::setCameraPose(glm::vec3 eye, glm::vec3 target, glm::vec3 up) {
    auto candidate = model_.camera_;
    candidate.lookAt(eye, target, up);
    model_.camera_ = candidate;
}
void EditorController::setStyle(StyleKind style) {
    if (style != StyleKind::Basic && style != StyleKind::Wireframe)
        throw std::invalid_argument("invalid render style");
    model_.style_ = style;
}
void EditorController::setRenderOptions(RenderOptions options) {
    for (int i = 0; i < 4; ++i)
        if (!std::isfinite(options.background[i]) || options.background[i] < 0 ||
            options.background[i] > 1)
            throw std::invalid_argument("background must be finite and normalized");
    // Selection has its own validated operation; render flags cannot bypass it.
    options.selected = model_.selected();
    model_.options_ = options;
}
void EditorController::setLightingSettings(LightingSettings settings) {
    settings.validate();
    model_.lighting_ = settings;
}
LightId EditorController::addDirectionalLight(DirectionalLight light) {
    return model_.lights_.addDirectional(std::move(light));
}
LightId EditorController::addPointLight(PointLight light) {
    return model_.lights_.addPoint(std::move(light));
}
LightId EditorController::addSpotLight(SpotLight light) {
    return model_.lights_.addSpot(std::move(light));
}
void EditorController::updateLight(LightId id, LightValue value) {
    model_.lights_.update(id, std::move(value));
}
void EditorController::removeLight(LightId id) {
    model_.lights_.remove(id);
}
void EditorController::clearLights() {
    model_.lights_.clear();
}
void EditorController::setPostProcessSettings(PostProcessSettings settings) {
    settings.validate();
    model_.postProcess_ = settings;
}
} // namespace luma
