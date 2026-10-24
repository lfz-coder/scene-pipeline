#include <luma/qt/ProjectDocument.hpp>
#include <map>
#include <stdexcept>
#include <type_traits>

namespace luma {
namespace {
ProjectObject captureObject(const EditorModel& model, const AssetStore& assets, const RenderItem& item) {
    const auto& transform = model.scene().transform(item.entity);
    const auto& material = model.materials().material(item.material);
    ProjectObject object;
    object.position = transform.position();
    object.scale = transform.scale();
    object.rotation = transform.rotation();
    object.color = material.baseColor;
    object.specularColor = material.specularColor;
    object.shininess = material.shininess;
    object.mix = material.textureMix;
    object.cutoff = material.alphaCutoff;
    object.alpha = material.alphaMode;
    object.doubleSided = material.doubleSided;
    object.baseTexture = ProjectTexture::capture(assets, material.baseColorTexture);
    object.secondaryTexture =
        ProjectTexture::capture(assets, material.secondaryColorTexture);
    object.specularTexture = ProjectTexture::capture(assets, material.specularTexture);
    object.materialGroup = item.material.value;
    return object;
}
} // namespace

ProjectDocument ProjectDocument::capture(const EditorModel& model, const AssetStore& assets,
                                         const std::string& presetName,
                                         const QString& title) {
    ProjectDocument document;
    document.name = title;
    document.preset = presetName;
    document.eye = model.camera().eye();
    document.target = model.camera().target();
    document.up = model.camera().up();
    document.cameraMode = model.controls().mode();
    document.style = model.style();
    document.options = model.options();
    document.lighting = model.lighting();
    document.lights = model.lights();
    document.postProcess = model.postProcess();
    for (const auto& item : model.scene().renderItems())
        document.objects.push_back(captureObject(model, assets, item));
    return document;
}

void ProjectDocument::apply(EditorController& controller, const EditorModel& model,
                            AssetStore& assets) const {
    if (objects.size() != model.scene().renderItems().size())
        throw std::runtime_error("工程对象数量与当前预设不匹配");
    std::map<unsigned, MaterialId> groups;
    for (std::size_t i = 0; i < objects.size(); ++i) {
        const auto& object = objects[i];
        controller.select(model.scene().renderItems()[i].entity);
        controller.setTransform(object.position, object.rotation, object.scale);
        if (!groups.count(object.materialGroup))
            groups[object.materialGroup] = controller.cloneSelectedMaterial();
        else
            controller.assignMaterial(groups.at(object.materialGroup));
        auto material = model.materials().material(model.selectedMaterial());
        material.baseColor = object.color;
        material.specularColor = object.specularColor;
        material.shininess = object.shininess;
        material.textureMix = object.mix;
        material.alphaCutoff = object.cutoff;
        material.alphaMode = object.alpha;
        material.doubleSided = object.doubleSided;
        if (version >= 2) {
            auto resolve = [&](const auto& texture) -> std::optional<TextureId> {
                return texture ? std::optional<TextureId>(texture->resolve(assets)) : std::nullopt;
            };
            material.baseColorTexture = resolve(object.baseTexture);
            material.secondaryColorTexture = resolve(object.secondaryTexture);
            material.specularTexture = resolve(object.specularTexture);
        }
        controller.setMaterial(material);
    }
    controller.clearLights();
    for (const auto& entry : lights.entries())
        std::visit(
            [&controller](const auto& light) {
                using T = std::decay_t<decltype(light)>;
                if constexpr (std::is_same_v<T, DirectionalLight>)
                    controller.addDirectionalLight(light);
                if constexpr (std::is_same_v<T, PointLight>)
                    controller.addPointLight(light);
                if constexpr (std::is_same_v<T, SpotLight>)
                    controller.addSpotLight(light);
            },
            entry.value);
    controller.setLightingSettings(lighting);
    controller.setPostProcessSettings(postProcess);
    controller.setCameraPose(eye, target, up);
    controller.setCameraMode(cameraMode);
    controller.setStyle(style);
    controller.setRenderOptions(options);
    if (!model.scene().renderItems().empty())
        controller.select(model.scene().renderItems().front().entity);
}
} // namespace luma
