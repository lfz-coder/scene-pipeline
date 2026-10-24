#include <luma/qt/WorkbenchWindow.hpp>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QListWidget>
#include <type_traits>
namespace luma {
void WorkbenchWindow::editLighting(int field) {
    if (refreshing_ || !viewport_->model()) return;
    auto settings = viewport_->model()->lighting();
    if (field == 0) settings.model = lightingModel_->currentIndex() == 1 ? ShadingModel::BlinnPhong : ShadingModel::Unlit;
    if (field == 1) settings.ambientStrength = ambient_->value();
    if (field == 2) settings.visualizeNormals = normalViz_->isChecked();
    if (invoke([settings](auto& c) { c.setLightingSettings(settings); }))
        changed(WorkflowStage::Lighting);
}
void WorkbenchWindow::addLight() {
    if (!viewport_->controller()) return;
    LightId id;
    const auto type = lightKind_->currentIndex();
    if (invoke([&](auto& c) {
        if (type == 0) id = c.addDirectionalLight({});
        if (type == 1) id = c.addPointLight({});
        if (type == 2) id = c.addSpotLight({});
    })) {
        changed(WorkflowStage::Lighting);
        addTask("添加灯光 · " + lightKind_->currentText(), true);
        refreshLighting();
        for (int i = 0; i < lightList_->count(); ++i)
            if (lightList_->item(i)->data(Qt::UserRole).toUInt() == id.value)
                lightList_->setCurrentRow(i);
    }
}
void WorkbenchWindow::removeLight() {
    if (!lightList_->currentItem() || !lightList_->currentItem()->isSelected()) return;
    const auto id = LightId{lightList_->currentItem()->data(Qt::UserRole).toUInt()};
    if (invoke([id](auto& c) { c.removeLight(id); })) {
        changed(WorkflowStage::Lighting);
        addTask("删除灯光 #" + QString::number(id.value), true);
        refreshLighting();
    }
}
void WorkbenchWindow::editLight(LightField field, int component) {
    if (refreshing_ || !viewport_->model() || !lightList_->currentItem() ||
        !lightList_->currentItem()->isSelected()) return;
    const auto id = LightId{lightList_->currentItem()->data(Qt::UserRole).toUInt()};
    auto value = viewport_->model()->lights().entry(id).value;
    std::visit([&](auto& light) {
        using T = std::decay_t<decltype(light)>;
        if (field == LightField::Enabled) light.enabled = lightEnabled_->isChecked();
        if (field == LightField::Intensity) light.intensity = lightIntensity_->value();
        if (field == LightField::Color) light.color[component] = lightColor_[component]->value();
        if constexpr (!std::is_same_v<T, DirectionalLight>) {
            if (field == LightField::Position)
                light.position[component] = lightPosition_[component]->value();
            if (field == LightField::Attenuation) {
                const float next = lightAttenuation_[component]->value();
                if (component == 0) light.constant = next;
                if (component == 1) light.linear = next;
                if (component == 2) light.quadratic = next;
            }
        }
        if constexpr (!std::is_same_v<T, PointLight>) {
            if (field == LightField::Direction)
                light.direction[component] = lightDirection_[component]->value();
        }
        if constexpr (std::is_same_v<T, SpotLight>) {
            if (field == LightField::Cone && component == 0)
                light.innerCutoffDegrees = lightCone_[0]->value();
            if (field == LightField::Cone && component == 1)
                light.outerCutoffDegrees = lightCone_[1]->value();
        }
    }, value);
    if (invoke([id, value](auto& c) { c.updateLight(id, value); }))
        changed(WorkflowStage::Lighting);
}
} // namespace luma
