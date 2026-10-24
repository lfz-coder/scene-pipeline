#include <cmath>
#include <luma/scene/MaterialDesc.hpp>
#include <stdexcept>
namespace luma {
void MaterialDesc::validate() const {
    if (alphaMode != AlphaMode::Opaque && alphaMode != AlphaMode::Mask &&
        alphaMode != AlphaMode::Blend)
        throw std::invalid_argument("invalid alpha mode");
    for (int i = 0; i < 4; ++i)
        if (!std::isfinite(baseColor[i]) || baseColor[i] < 0 || baseColor[i] > 1)
            throw std::invalid_argument("baseColor must be finite linear RGBA in [0,1]");
    for (int i = 0; i < 3; ++i)
        if (!std::isfinite(specularColor[i]) || specularColor[i] < 0 || specularColor[i] > 1)
            throw std::invalid_argument("specularColor must be finite linear RGB in [0,1]");
    if (!std::isfinite(shininess) || shininess < 1 || shininess > 256)
        throw std::invalid_argument("shininess must be finite in [1,256]");
    if (!std::isfinite(alphaCutoff) || alphaCutoff < 0 || alphaCutoff > 1)
        throw std::invalid_argument("alphaCutoff must be in [0,1]");
    if (baseColorTexture && !baseColorTexture->value)
        throw std::invalid_argument("invalid baseColorTexture ID 0");
    if (!std::isfinite(textureMix) || textureMix < 0 || textureMix > 1)
        throw std::invalid_argument("textureMix must be finite in [0,1]");
    if (secondaryColorTexture && !baseColorTexture)
        throw std::invalid_argument("secondaryColorTexture requires baseColorTexture");
    if (secondaryColorTexture && !secondaryColorTexture->value)
        throw std::invalid_argument("invalid secondaryColorTexture ID 0");
    if (specularTexture && !specularTexture->value)
        throw std::invalid_argument("invalid specularTexture ID 0");
}
} // namespace luma
