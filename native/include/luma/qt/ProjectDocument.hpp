#pragma once
#include <QString>
#include <QJsonObject>
#include <luma/editor/EditorController.hpp>
#include <luma/lighting/LightRig.hpp>
#include <luma/qt/ProjectTexture.hpp>
#include <luma/render/PostProcessSettings.hpp>
#include <vector>
namespace luma {
struct ProjectObject {
    glm::vec3 position{}, scale{1};
    glm::quat rotation{1, 0, 0, 0};
    glm::vec4 color{1};
    glm::vec3 specularColor{.5f};
    float shininess = 32;
    float mix = .5f, cutoff = .5f;
    std::optional<ProjectTexture> baseTexture, secondaryTexture, specularTexture;
    AlphaMode alpha = AlphaMode::Opaque;
    bool doubleSided = false;
    unsigned materialGroup = 0;
};
// Versioned scene recipe. GPU handles and Qt widget state never enter the file.
struct ProjectDocument {
    int version = 2;
    QString name = "未命名作品";
    std::string preset = "robot";
    glm::vec3 eye{5, 3.3f, 7}, target{0, .7f, 0}, up{0, 1, 0};
    CameraMode cameraMode = CameraMode::Orbit;
    StyleKind style = StyleKind::Basic;
    RenderOptions options;
    LightingSettings lighting;
    LightRig lights;
    PostProcessSettings postProcess;
    std::vector<ProjectObject> objects;
    QJsonObject studio; // Optional v2 extension; never contains service credentials.
    static ProjectDocument capture(const EditorModel&, const AssetStore&, const std::string& preset,
                                   const QString& name);
    static ProjectDocument read(const QString& path);
    void write(const QString& path) const;
    void apply(EditorController&, const EditorModel&, AssetStore&) const;
};
} // namespace luma
