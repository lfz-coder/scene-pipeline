#include "ProjectTextureJson.hpp"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <cmath>
#include <luma/qt/ProjectDocument.hpp>
#include <luma/studio/StudioDocument.hpp>
#include <map>
#include <stdexcept>

namespace luma {
namespace {
[[noreturn]] void invalid() {
    throw std::runtime_error("工程文件不完整或版本不兼容");
}
float number(const QJsonValue& value) {
    if (!value.isDouble() || !std::isfinite(value.toDouble()) || std::abs(value.toDouble()) > 1e8)
        invalid();
    return static_cast<float>(value.toDouble());
}
int integer(const QJsonValue& value, int low, int high) {
    const auto n = number(value);
    if (n < low || n > high || n != std::floor(n))
        invalid();
    return static_cast<int>(n);
}
unsigned idValue(const QJsonValue& value) {
    return static_cast<unsigned>(integer(value, 0, 100000000));
}
bool boolean(const QJsonValue& value) {
    if (!value.isBool())
        invalid();
    return value.toBool();
}
template <int N> glm::vec<N, float> vector(const QJsonValue& value) {
    const auto values = value.toArray();
    if (!value.isArray() || values.size() != N)
        invalid();
    glm::vec<N, float> result;
    for (int i = 0; i < N; ++i)
        result[i] = number(values[i]);
    return result;
}
template <int N> QJsonArray array(glm::vec<N, float> value) {
    QJsonArray result;
    for (int i = 0; i < N; ++i)
        result.append(value[i]);
    return result;
}
void writeLight(QJsonArray& target, const LightEntry& entry) {
    QJsonObject object;
    std::visit(
        [&object](const auto& light) {
            using T = std::decay_t<decltype(light)>;
            object["enabled"] = light.enabled;
            object["color"] = array(light.color);
            object["intensity"] = light.intensity;
            if constexpr (std::is_same_v<T, DirectionalLight>) {
                object["type"] = "directional";
                object["direction"] = array(light.direction);
            } else if constexpr (std::is_same_v<T, PointLight>) {
                object["type"] = "point";
                object["position"] = array(light.position);
                object["constant"] = light.constant;
                object["linear"] = light.linear;
                object["quadratic"] = light.quadratic;
            } else {
                object["type"] = "spot";
                object["position"] = array(light.position);
                object["direction"] = array(light.direction);
                object["constant"] = light.constant;
                object["linear"] = light.linear;
                object["quadratic"] = light.quadratic;
                object["inner"] = light.innerCutoffDegrees;
                object["outer"] = light.outerCutoffDegrees;
            }
        },
        entry.value);
    target.append(object);
}
void readLight(LightRig& rig, const QJsonValue& value) {
    if (!value.isObject())
        invalid();
    const auto object = value.toObject();
    const auto type = object["type"].toString();
    const auto color = vector<3>(object["color"]);
    const auto intensity = number(object["intensity"]);
    const auto enabled = boolean(object["enabled"]);
    if (type == "directional") {
        rig.addDirectional({vector<3>(object["direction"]), color, intensity, enabled});
    } else if (type == "point") {
        rig.addPoint({vector<3>(object["position"]), color, intensity,
                      number(object["constant"]), number(object["linear"]),
                      number(object["quadratic"]), enabled});
    } else if (type == "spot") {
        rig.addSpot({vector<3>(object["position"]), vector<3>(object["direction"]), color,
                     intensity, number(object["constant"]), number(object["linear"]),
                     number(object["quadratic"]), number(object["inner"]),
                     number(object["outer"]), enabled});
    } else {
        invalid();
    }
}
} // namespace

void ProjectDocument::write(const QString& path) const {
    QJsonArray items;
    for (const auto& object : objects)
        items.append(QJsonObject{
            {"position", array(object.position)},
            {"scale", array(object.scale)},
            {"rotation", QJsonArray{object.rotation.w, object.rotation.x, object.rotation.y,
                                    object.rotation.z}},
            {"color", array(object.color)},
            {"specularColor", array(object.specularColor)},
            {"shininess", object.shininess},
            {"mix", object.mix},
            {"cutoff", object.cutoff},
            {"alpha", int(object.alpha)},
            {"doubleSided", object.doubleSided},
            {"baseTexture", writeTexture(object.baseTexture)},
            {"secondaryTexture", writeTexture(object.secondaryTexture)},
            {"specularTexture", writeTexture(object.specularTexture)},
            {"group", int(object.materialGroup)}});
    QJsonArray lightArray;
    for (const auto& entry : lights.entries())
        writeLight(lightArray, entry);
    const QJsonObject lightingObject{
        {"model", int(lighting.model)},
        {"ambientColor", array(lighting.ambientColor)},
        {"ambientStrength", lighting.ambientStrength},
        {"visualizeNormals", lighting.visualizeNormals},
        {"maxLights", lighting.maxLights}};
    const QJsonObject postObject{{"enabled", postProcess.enabled},
                                 {"effect", int(postProcess.effect)},
                                 {"intensity", postProcess.intensity},
                                 {"radius", postProcess.radius}};
    QJsonObject root{
        {"version", 2},
        {"name", name},
        {"preset", QString::fromStdString(preset)},
        {"eye", array(eye)},
        {"target", array(target)},
        {"up", array(up)},
        {"cameraMode", int(cameraMode)},
        {"style", int(style)},
        {"background", array(options.background)},
        {"depth", options.depth},
        {"cull", options.cull},
        {"blend", options.blend},
        {"outline", options.outline},
        {"lighting", lightingObject},
        {"lights", lightArray},
        {"postProcess", postObject},
        {"objects", items}};
    if (!studio.isEmpty()) {
        StudioDocument::fromJson(studio);
        root["studio"] = studio;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        throw std::runtime_error(file.errorString().toStdString());
    const auto bytes = QJsonDocument(root).toJson();
    if (bytes.size() > 16 * 1024 * 1024) throw std::runtime_error("工程文件超过 16 MiB，请减少内嵌快照");
    if (file.write(bytes) != bytes.size() || !file.commit())
        throw std::runtime_error(file.errorString().toStdString());
}

ProjectDocument ProjectDocument::read(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error(file.errorString().toStdString());
    if (file.size() > 16 * 1024 * 1024)
        invalid();
    QJsonParseError error;
    const auto json = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !json.isObject())
        invalid();
    const auto root = json.object();
    const auto version = integer(root["version"], 1, 2);
    ProjectDocument document;
    document.version = version;
    if (root.contains("studio")) {
        if (!root["studio"].isObject()) invalid();
        StudioDocument::fromJson(root["studio"].toObject());
        document.studio = root["studio"].toObject();
    }
    document.name = root["name"].toString().trimmed();
    document.preset = root["preset"].toString().toStdString();
    const std::map<std::string, int> counts{{"robot", 10}, {"camera", 10}, {"window", 0},
                                            {"triangle", 1}, {"quad", 1}, {"texture", 4},
                                            {"state", 6}, {"lighting", 4}};
    if (!root["name"].isString() || !root["preset"].isString() || document.name.isEmpty() ||
        document.name.size() > 120 || !counts.count(document.preset) ||
        !root["objects"].isArray())
        invalid();
    document.eye = vector<3>(root["eye"]);
    document.target = vector<3>(root["target"]);
    document.up = vector<3>(root["up"]);
    Camera camera;
    camera.lookAt(document.eye, document.target, document.up);
    document.cameraMode = CameraMode(integer(root["cameraMode"], 0, 1));
    document.style = StyleKind(integer(root["style"], 0, 1));
    document.options.background = vector<4>(root["background"]);
    for (int i = 0; i < 4; ++i)
        if (document.options.background[i] < 0 || document.options.background[i] > 1)
            invalid();
    document.options.depth = boolean(root["depth"]);
    document.options.cull = boolean(root["cull"]);
    document.options.blend = boolean(root["blend"]);
    document.options.outline = boolean(root["outline"]);
    if (version == 2) {
        const auto lightingObject = root["lighting"].toObject();
        document.lighting.model = ShadingModel(integer(lightingObject["model"], 0, 1));
        document.lighting.ambientColor = vector<3>(lightingObject["ambientColor"]);
        document.lighting.ambientStrength = number(lightingObject["ambientStrength"]);
        document.lighting.visualizeNormals = boolean(lightingObject["visualizeNormals"]);
        document.lighting.maxLights = integer(lightingObject["maxLights"], 1, 13);
        document.lighting.validate();
        const auto lightArray = root["lights"].toArray();
        if (!root["lights"].isArray() || lightArray.size() > 13)
            invalid();
        for (const auto& value : lightArray)
            readLight(document.lights, value);
        const auto postObject = root["postProcess"].toObject();
        document.postProcess.enabled = boolean(postObject["enabled"]);
        document.postProcess.effect = PostProcessEffect(integer(postObject["effect"], 0, 4));
        document.postProcess.intensity = number(postObject["intensity"]);
        document.postProcess.radius = integer(postObject["radius"], 1, 15);
        document.postProcess.validate();
    }
    document.lights.validate();
    const auto items = root["objects"].toArray();
    if (items.size() != counts.at(document.preset))
        invalid();
    std::map<unsigned, ProjectObject> groups;
    for (const auto& value : items) {
        if (!value.isObject())
            invalid();
        const auto object = value.toObject();
        ProjectObject parsed;
        parsed.position = vector<3>(object["position"]);
        parsed.scale = vector<3>(object["scale"]);
        const auto rotation = vector<4>(object["rotation"]);
        parsed.rotation = {rotation[0], rotation[1], rotation[2], rotation[3]};
        Transform transform;
        transform.setPosition(parsed.position);
        transform.setScale(parsed.scale);
        transform.setRotation(parsed.rotation);
        parsed.color = vector<4>(object["color"]);
        if (version == 2) {
            parsed.specularColor = vector<3>(object["specularColor"]);
            parsed.shininess = number(object["shininess"]);
            parsed.baseTexture = readTexture(object["baseTexture"]);
            parsed.secondaryTexture = readTexture(object["secondaryTexture"]);
            parsed.specularTexture = readTexture(object["specularTexture"]);
        }
        parsed.mix = number(object["mix"]);
        parsed.cutoff = number(object["cutoff"]);
        parsed.alpha = AlphaMode(integer(object["alpha"], 0, 2));
        parsed.doubleSided = boolean(object["doubleSided"]);
        parsed.materialGroup = idValue(object["group"]);
        if (!parsed.materialGroup)
            invalid();
        MaterialDesc material;
        material.baseColor = parsed.color;
        material.specularColor = parsed.specularColor;
        material.shininess = parsed.shininess;
        material.textureMix = parsed.mix;
        material.alphaCutoff = parsed.cutoff;
        material.alphaMode = parsed.alpha;
        material.doubleSided = parsed.doubleSided;
        // Validate slot relationships before a running scene can be replaced.
        if (parsed.baseTexture) material.baseColorTexture = TextureId{1};
        if (parsed.secondaryTexture) material.secondaryColorTexture = TextureId{1};
        if (parsed.specularTexture) material.specularTexture = TextureId{1};
        material.validate();
        if (groups.count(parsed.materialGroup)) {
            const auto& previous = groups.at(parsed.materialGroup);
            if (previous.color != parsed.color || previous.specularColor != parsed.specularColor ||
                previous.shininess != parsed.shininess || previous.mix != parsed.mix ||
                previous.cutoff != parsed.cutoff || previous.alpha != parsed.alpha ||
                previous.doubleSided != parsed.doubleSided ||
                previous.baseTexture != parsed.baseTexture ||
                previous.secondaryTexture != parsed.secondaryTexture ||
                previous.specularTexture != parsed.specularTexture)
                invalid();
        } else {
            groups[parsed.materialGroup] = parsed;
        }
        document.objects.push_back(parsed);
    }
    return document;
}
} // namespace luma
