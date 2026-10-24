#include "ProjectTextureJson.hpp"
#include <QJsonObject>
#include <luma/app/Application.hpp>
#include <stdexcept>

namespace luma {
void ProjectTexture::validate() const {
    if (asset != "textures/checker.png" && asset != "textures/cutout.png" &&
        asset != "textures/specular-grid.png")
        throw std::invalid_argument("工程纹理必须引用受支持的内置资源");
    options.validate();
}
std::optional<ProjectTexture> ProjectTexture::capture(const AssetStore& assets,
                                                       std::optional<TextureId> id) {
    if (!id) return std::nullopt;
    const auto& source = assets.textureSource(*id);
    ProjectTexture result{std::filesystem::relative(source.path, assetRoot()).generic_string(),
                          source.options};
    result.validate();
    return result;
}
TextureId ProjectTexture::resolve(AssetStore& assets) const {
    validate();
    return assets.loadTexture(assetRoot() / asset, options);
}
QJsonValue writeTexture(const std::optional<ProjectTexture>& texture) {
    if (!texture) return QJsonValue(QJsonValue::Null);
    texture->validate();
    const auto& options = texture->options;
    return QJsonObject{{"asset", QString::fromStdString(texture->asset)},
                       {"colorSpace", int(options.colorSpace)}, {"flipY", options.flipY},
                       {"wrap", int(options.wrap)}, {"minFilter", int(options.minFilter)},
                       {"magFilter", int(options.magFilter)}, {"mipmaps", options.mipmaps}};
}
std::optional<ProjectTexture> readTexture(const QJsonValue& value) {
    if (value.isNull()) return std::nullopt;
    if (!value.isObject()) throw std::invalid_argument("工程纹理描述不完整");
    const auto object = value.toObject();
    auto integer = [&](const char* key, int maximum) {
        const auto entry = object[key];
        if (!entry.isDouble() || entry.toDouble() < 0 || entry.toDouble() > maximum ||
            entry.toDouble() != entry.toInt())
            throw std::invalid_argument("工程纹理参数无效");
        return entry.toInt();
    };
    if (!object["asset"].isString() || !object["flipY"].isBool() || !object["mipmaps"].isBool())
        throw std::invalid_argument("工程纹理参数不完整");
    ProjectTexture result;
    result.asset = object["asset"].toString().toStdString();
    result.options.colorSpace = ColorSpace(integer("colorSpace", 1));
    result.options.wrap = Wrap(integer("wrap", 1));
    result.options.minFilter = Filter(integer("minFilter", 2));
    result.options.magFilter = Filter(integer("magFilter", 1));
    result.options.flipY = object["flipY"].toBool();
    result.options.mipmaps = object["mipmaps"].toBool();
    result.validate();
    return result;
}
} // namespace luma
